// (c) 2023 Johnson Controls.  All Rights reserved.

'use strict';

var os = require('os');
const fs = require('fs');
var nodeStatic = require('node-static');
var http = require('https');
var socketIO = require('socket.io');
const config = require('./config');
const express = require('express');

const {
    v4: uuidV4
} = require('uuid');

let webServer;
let socketServer;
let expressApp;
let io;

(async () => {
    try {
        await runExpressApp();
        await runWebServer();
        await runSocketServer();
    } catch (err) {
        console.error(err);
    }
})();

console.log("To browse https://localhost/ or https://ip/");

let serverSocketid = null;

var fileServer = new(nodeStatic.Server)();

async function runExpressApp() {
    expressApp = express();
    expressApp.use(express.json());
    expressApp.use(express.static(__dirname));

    expressApp.use((error, req, res, next) => {
        if (error) {
            console.log('Express app error,', error.message);

            error.status = error.status || (error.name === 'TypeError' ? 400 : 500);
            res.statusMessage = error.message;
            res.status(error.status).send(String(error));
        } else {
            next();
        }
    });
}

async function runWebServer() {
    console.error('runWebServer');

    const {
        sslKey,
        sslCrt
    } = config;
    if (!fs.existsSync(sslKey) || !fs.existsSync(sslCrt)) {
        console.error('SSL files are not found. check your config.js file');
        process.exit(0);
    }
    const tls = {
        cert: fs.readFileSync(sslCrt),
        key: fs.readFileSync(sslKey),
    };
    
    webServer = http.createServer(tls, expressApp);
    //webServer = http.createServer(expressApp); // for http
    webServer.on('error', (err) => {
        console.error('starting web server failed:', err.message);
    });

    await new Promise((resolve) => {
        const {
            listenIp,
            listenPort
        } = config;
        webServer.listen(listenPort, listenIp, () => {
            console.log('server is running');
            console.log(`open https://127.0.0.1:${listenPort} in your web browser`);
            resolve();
        });
    });
}

async function runSocketServer() {
    console.error('runSocketServer');
    io = socketIO.listen(webServer);

    io.sockets.on('connection', function(socket) {
        // convenience function to log server messages on the client
        function log() {
            var array = ['Message from server:'];
            array.push.apply(array, arguments);
	    // Uncomment to see server side messages on client side JavaScript console.
            // socket.emit('log', array);
            console.log(array);
        }

        socket.on('disconnect', function() {
            if (socket.id == serverSocketid) {

                console.log("server down " + serverSocketid);
                serverSocketid = null;

                for (let soc in io.sockets.connected) {
                    io.sockets.connected[soc].emit('leave', socket.room, -1, -1);
                    io.sockets.connected[soc].disconnect();
                }
            } else {
                var clientsInRoom = io.sockets.adapter.rooms[socket.room];
                var numClients = clientsInRoom ? Object.keys(clientsInRoom.sockets).length : 0;
                console.log("disconnect " + socket.id + " from room " + socket.room + " numClients " + numClients);

                if (serverSocketid && io.sockets.connected[serverSocketid]) {
		    /* Sometimes socket id of webrtc remains the same after service restart.
                       If detected, disconnect obsolete socket */
                    io.sockets.connected[serverSocketid].emit('disconnectClient', socket.id);
		}

                io.sockets.to(socket.room).emit('leave', socket.room, socket.id, numClients);
                console.log("unsubscribe " + socket.id);
            }
        });

        socket.on('WebrtcSocket', function() {
            log('Received request to create WebrtcSocket');

            if (serverSocketid !== null && io.sockets.connected[serverSocketid]) {
                io.sockets.connected[serverSocketid].disconnect();
                serverSocketid = null;
            }

            log('Webrtc ID ', socket.id);
            serverSocketid = socket.id;
        });

        socket.on('create or join', function(roomId, user) {
            log('Received request to create or join room ' + roomId);

            if (serverSocketid == null || io.sockets.connected[serverSocketid] == null) {
                io.emit('leave', socket.room, -1, -1);
                return console.error("Webrtc server is down");
            }

            if (socket.room)
                socket.leave(socket.room);

            socket.room = roomId;
            socket.join(roomId);

            if (user)
                socket.user = user;

            var numClients = io.sockets.adapter.rooms[roomId].length; //For socket.io versions >= 1.4:

            log('Room ' + roomId + ' now has ' + numClients + ' client(s)');

            if (numClients === 1) {
                log('Client ID ' + socket.id + ' created room ' + roomId);
                //when first time client connection then socket is created, store socket id and then emit  Join event
                io.sockets.connected[serverSocketid].emit('created', roomId, serverSocketid);
                socket.emit('created', roomId, socket.id);
                socket.emit('joined', roomId, socket.id, numClients);
            } else if (numClients > 1) {
                log('Client ID ' + socket.id + ' joined room ' + roomId);
                //if already client connections are there then we send event Joined event.
                
                // when all the users in room need get joining event
                //io.sockets.in(roomId).emit('join', roomId, socket.id, numClients);

                socket.emit('joined', roomId, socket.id, numClients);

            }
        });

        socket.on('message', function(message) {
            console.log('SFU server message: ', message);

            message.from = socket.id;

            if (socket.user)
                message.user = socket.user;

            socket.to(message.to).emit('message', message);
        });

        socket.on('messageToWebrtc', function(message) {
            message.from = socket.id;

            if (socket.user)
                message.user = socket.user;

            console.log('app message: ', message);
            if (io.sockets.connected[serverSocketid])
                io.sockets.connected[serverSocketid].emit('message', message);
        });

        /* To broadcast to all in a roomm including sender: socket.in(room).emit();
	   To exclude sender: socket.to(); */
        socket.on('postAppMessage', function(message) {
            if (message.type === "user") {
                message.user = message.desc;
            }

            console.log('notification ' + JSON.stringify(message, null, 4));
            message.from = socket.id;

            if (socket.user)
                message.user = socket.user;

            if (message.type === "chat") {
                if ('room' in message) {
                    io.in(message.room).emit('message', message);
                }
            } else {
                //if ('room' in message) {
                socket.to(message.to).emit('message', message);
                //}
            }
        });

        socket.on('bye', function() {
            console.log('received bye');
        });

    });
}
