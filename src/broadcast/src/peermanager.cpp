

#include "webrtc/peermanager.h"
//#include "base/memory.h"


using std::endl;


namespace base
{
namespace web_rtc
{

PeerManager::PeerManager()  // (PeerFactoryContext* context)
// : _context(context)
{
    SInfo << "PeerManager()";
}

PeerManager::~PeerManager()
{
    SInfo << "PeerManager()";
}

void PeerManager::sendSDP(Peer *conn, const std::string &type, const std::string &sdp)
{
    assert(0 && "virtual");
}

void PeerManager::postAppMessage(std::string message, std::string from, std::string &room)
{
    assert(0 && "virtual");
}

void PeerManager::sendCandidate(Peer *conn, const std::string &mid, int mlineindex, const std::string &sdp)
{
    assert(0 && "virtual");
}

void PeerManager::recvSDP(const std::string &token, const json &message)
{
    auto conn = PeerManager::get(token, false);
    if (!conn)
    {
        assert(0 && "peer mismath");
        return;
    }

    std::string type = message.value(kSessionDescriptionTypeName, "");
    std::string sdp = message.value(kSessionDescriptionSdpName, "");
    if (sdp.empty() || (type != "offer" && type != "answer"))
    {
        SError << "Received bad sdp: " <<  type <<  ": " <<  sdp; 
        assert(0 && "bad sdp");
        return;
    }

    en_EncType encType = EN_NONE;  // parse sdp and set the correct  EncType

    std::string vtrackid;

    std::string atrackid;

    conn->recvSDP(type, sdp, encType, vtrackid, atrackid);

   // _capturer.onAnswer(conn, encType, vtrackid, atrackid); // arvind

    LDebug("Received ", type, ": ", sdp)
}

void PeerManager::recvCandidate(const std::string &token, const json &message)
{
    auto conn = PeerManager::get(token, false);
    if (!conn)
    {
        assert(0 && "peer mismath");
        return;
    }

    LInfo("recvCandidate", cnfg::stringify(message))

        std::string mid
        = message.value(kCandidateSdpMidName, "");
    int mlineindex = message.value(kCandidateSdpMlineIndexName, -1);
    std::string sdp = message.value(kCandidateSdpName, "");
    if (mlineindex == -1 || mid.empty() || sdp.empty())
    {
        LError("Invalid candidate format")
            // assert(0 && "bad candiate");
            return;
    }

    LDebug("Received candidate: ", sdp)

        conn->recvCandidate(mid, mlineindex, sdp);
}

void PeerManager::onAddRemoteStream(Peer *conn, webrtc::MediaStreamInterface *stream)
{
    //  assert(0 && "virtual");
}

void PeerManager::onRemoveRemoteStream(Peer *conn, webrtc::MediaStreamInterface *stream)
{
    assert(0 && "virtual");
}

void PeerManager::onStable(Peer *conn) {}

void PeerManager::onClosed(Peer *conn)
{
    LInfo("Deleting peer connection: ", conn->peerid())
}

void PeerManager::onFailure(Peer *conn, const std::string &error)
{
    LInfo("Deleting peer connection: ", conn->peerid())
        // arvind  // probably remove teh following line and move it to singaller.cpp, it shoudl be simillar to
        // onclose
        if (remove(conn)) delete conn;
    //   deleteLater<Peer>(conn); // async delete
}

//        void PeerManager::oncommand(const std::string& token, std::string& cmd,  std::string& para)
//        {
//            auto conn = PeerManager::get(token, false);
//           _capturer.oncommand(conn , cmd , para);
//        }


}  // namespace web_rtc
}  // namespace base
