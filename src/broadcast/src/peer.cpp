
#include "webrtc/peer.h"
#include "webrtc/peermanager.h"
#include "webrtc/peerfactorycontext.h"
#include "base/logger.h"
#include "p2p/base/transport_info.h"
#include "pc/media_session.h"
#include "pc/peer_connection_wrapper.h"
#include "pc/sdp_utils.h"


using std::endl;

namespace base
{
namespace web_rtc
{


cricket::Candidate CreateLocalUdpCandidate(const rtc::SocketAddress &address)
{
    cricket::Candidate candidate;
    candidate.set_component(cricket::ICE_CANDIDATE_COMPONENT_DEFAULT);
    candidate.set_protocol(cricket::UDP_PROTOCOL_NAME);
    candidate.set_address(address);
    candidate.set_type(cricket::LOCAL_PORT_TYPE);
    return candidate;
}

cricket::Candidate CreateLocalTcpCandidate(const rtc::SocketAddress &address)
{
    cricket::Candidate candidate;
    candidate.set_component(cricket::ICE_CANDIDATE_COMPONENT_DEFAULT);
    candidate.set_protocol(cricket::TCP_PROTOCOL_NAME);
    candidate.set_address(address);
    candidate.set_type(cricket::LOCAL_PORT_TYPE);
    candidate.set_tcptype(cricket::TCPTYPE_PASSIVE_STR);
    return candidate;
}


bool AddCandidateToFirstTransport(cricket::Candidate *candidate, webrtc::SessionDescriptionInterface *sdesc)
{
    auto *desc = sdesc->description();
    // RTC_DCHECK(desc->contents().size() > 0);
    const auto &first_content = desc->contents()[0];
    candidate->set_transport_name(first_content.name);
    std::unique_ptr<webrtc::IceCandidateInterface> jsep_candidate
        = webrtc::CreateIceCandidate(first_content.name, 0, *candidate);
    return sdesc->AddCandidate(jsep_candidate.get());
}


Peer::Peer(
    PeerManager *manager,
    PeerFactoryContext *context,
    std::string &cam,    
    std::string &room,
    const std::string &peerid,
    Mode mode)
    : cam(cam), room(room),
      _manager(manager),
      _context(context),
      _peerid(peerid),
      _mode(mode)
      //, _context->factory(manager->factory())
      ,
      _peerConnection(nullptr)
{
    SInfo << _peerid << ": Creating ";

    webrtc::PeerConnectionInterface::IceServer stun;
    // stun.uri = kGoogleStunServerUri;
    //_config.servers.push_back(stun);

    stun.uri = "stun:stun.l.google.com:19302";
    _config.servers.push_back(stun);
    //  config_.sdp_semantics = sdp_semantics;

    // _constraints.SetMandatoryReceiveAudio(true);
    // _constraints.SetMandatoryReceiveVideo(true);
    // _constraints.SetAllowDtlsSctpDataChannels();

    //_config.servers.clear();
    //_config.servers.empty();
    _config.enable_rtp_data_channel = false;
    _config.enable_dtls_srtp = true;
    _config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
    _config.rtcp_mux_policy = webrtc::PeerConnectionInterface::kRtcpMuxPolicyRequire;
    _config.bundle_policy = webrtc::PeerConnectionInterface::kBundlePolicyMaxBundle;
    _config.type = webrtc::PeerConnectionInterface::kAll;
    // _config.candidate_network_policy = webrtc::PeerConnectionInterface::kCandidateNetworkPolicyLowCost;
    // _config.min_port =80000;
    //_config.max_port =100000;
    // _config.enable_ice_renomination = true;
    //_config.ice_candidate_pool_size=1;
}


Peer::~Peer()
{
    // LInfo(_peerid, ": Destroying " , this )
    SInfo << "Destroying " << _peerid << " obj " << this;
    // closeConnection();  // Do not uncomment it it will cause memory leaks
}


// rtc::scoped_refptr<webrtc::MediaStreamInterface> Peer::createMediaStream()
//{
//    // assert(_mode == Offer);
//     //assert(_context->factory);
//     assert(!_stream);
//     _stream = _context->factory->CreateLocalMediaStream(kStreamLabel);
//     return _stream;
// }


// void Peer::setPortRange(int minPort, int maxPort)
// {
//     assert(!_peerConnection);

//     if (!_context->networkManager) {
//         throw std::runtime_error("Must initialize custom network manager to set port range");
//     }

//     if (!_context->socketFactory) {
//         throw std::runtime_error("Must initialize custom socket factory to set port range");
//     }

//     if (!_portAllocator)
//         _portAllocator.reset(new cricket::BasicPortAllocator(
//             _context->networkManager.get(),
//             _context->socketFactory.get()));
//     _portAllocator->SetPortRange(minPort, maxPort);
// }


void Peer::createConnection()
{
    assert(_context->factory);
    _peerConnection = _context->factory->CreatePeerConnection(_config, nullptr, nullptr, this);

    //    if (_stream) {
    //        if (!_peerConnection->AddStream(_stream)) {
    //            throw std::runtime_error("Adding stream to Peer failed");
    //        }
    //    }
}


void Peer::closeConnection()
{
    LInfo(_peerid, ": Closing")

        if (_peerConnection)
    {
        _peerConnection->Close();
    }
    else
    {
        // Call onClosed if no connection has been
        // made so callbacks are always run.
        _manager->onClosed(this);
    }
}


void Peer::createOffer( bool video , bool audio)
{
    // assert(_mode == Offer);
    // assert(_peerConnection);

    LInfo(_peerid, ": Create Offer")

    webrtc::PeerConnectionInterface::RTCOfferAnswerOptions options;

    options.offer_to_receive_audio = audio ;
    options.offer_to_receive_video = video;


    _peerConnection->CreateOffer(this, options);
}


void Peer::recvSDP(
    const std::string &type,
    const std::string &sdp,
    en_EncType &encType,
    std::string &vtrackid,
    std::string &atrackid)
{
    LDebug(_peerid, ": Received answer ", type, ": ", sdp)

        webrtc::SdpParseError error;
    webrtc::SessionDescriptionInterface *desc(webrtc::CreateSessionDescription(type, sdp, &error));
    if (!desc) { throw std::runtime_error("Can't parse remote SDP: " + error.description); }
    _peerConnection->SetRemoteDescription(DummySetSessionDescriptionObserver::Create(), desc);


    webrtc::PeerConnectionInterface::RTCOfferAnswerOptions options;
    options.offer_to_receive_audio = false;
    options.offer_to_receive_video = true;

    if (type == "offer")
    {
        // assert(_mode == Answer);
        SError << _peerid << ": wrong state Received " <<  type  <<  ": " << sdp;
        _peerConnection->CreateAnswer(this, options);
    }
    else
    {}

    std::vector<rtc::scoped_refptr<webrtc::RtpSenderInterface>> senders = _peerConnection->GetSenders();

    for (const rtc::scoped_refptr<webrtc::RtpSenderInterface> &sender : senders)
    {
        std::string str = sender->id();

        // cricket::MediaType media_type = sender->media_type();
        webrtc::MediaStreamTrackInterface *track = sender->track();
        if (!track) { continue; }

        if (track && track->kind() == "video")
        {
            vtrackid = track->id();  // removed prefixed vl
        }
        else if (track && track->kind() == "audio")
        {
            atrackid = track->id();  // removed prefixed vl
        }

        SInfo << track->kind() << " " << track->id();
        
        encType = EN_NATIVE;

        //webrtc::RtpParameters getParameters = sender->GetParameters();

      //  std::vector<webrtc::RtpCodecParameters> vtCodecs = getParameters.codecs;

//        for (webrtc::RtpCodecParameters &codecs : vtCodecs)
//        {
//            SDebug << codecs.name;
//            if (codecs.name == "H264")
//            {
//                std::unordered_map<std::string, std::string> parameters = codecs.parameters;
//
//                if (parameters["Enc"] == "NATIVE")
//                {
//                    encType = EN_NATIVE;
//                    break;
//                }
//                else if (parameters["Enc"] == "NVIDIA")
//                {
//                    encType = EN_NVIDIA;
//                    break;
//                }
//                else if (parameters["Enc"] == "QUICKSYNC")
//                {
//                    encType = EN_QUICKSYNC;
//                    break;
//                }
//                else if (parameters["Enc"] == "X264")  /// never use this better to use VP9
//                {
//                    encType = EN_X264;
//                    break;
//                }
//
//
//                //                for( auto &parameter :parameters)
//                //                {
//                //                     SInfo <<  parameter.first << " " <<  parameter.second;
//                //                }
//            }
//            else if (codecs.name == "VP9")
//            {
//                encType = EN_VP9;
//            }
//        }
    }
}


void Peer::recvCandidate(const std::string &mid, int mlineindex, const std::string &sdp)
{
    webrtc::SdpParseError error;
    std::unique_ptr<webrtc::IceCandidateInterface> candidate(
        webrtc::CreateIceCandidate(mid, mlineindex, sdp, &error));
    if (!candidate) { throw std::runtime_error("Can't parse remote candidate: " + error.description); }
    _peerConnection->AddIceCandidate(candidate.get());
}


void Peer::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state)
{
    LInfo(_peerid, ": On signaling state change: ", new_state)

        switch (new_state)
    {
    case webrtc::PeerConnectionInterface::kStable:
        _manager->onStable(this);
        break;
    case webrtc::PeerConnectionInterface::kClosed:
        // _manager->onClosed(this);  // Do not uncomment it, it will cause memory leaks
        break;
    case webrtc::PeerConnectionInterface::kHaveLocalOffer:
    case webrtc::PeerConnectionInterface::kHaveRemoteOffer:
    case webrtc::PeerConnectionInterface::kHaveLocalPrAnswer:
    case webrtc::PeerConnectionInterface::kHaveRemotePrAnswer:
        break;
    }
}


void Peer::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state)
{
    LDebug(_peerid, ": On ICE connection change: ", new_state)
}


void Peer::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state)
{
    LDebug(_peerid, ": On ICE gathering change: ", new_state)

    // if( new_state == webrtc::PeerConnectionInterface::kIceGatheringComplete)
    // {
    //     createOffer();
    //     hasIceLiteOffer=true;
    // }
}


void Peer::OnRenegotiationNeeded()
{
    LInfo(_peerid, ": On renegotiation needed")
}


void Peer::OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream)
{
    LInfo(_peerid, ": OnAddStream")
        // proxy to deprecated OnAddStream method
        OnAddStream(stream.get());
}

void Peer::OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver)
{
    LInfo(_peerid, ": OnTrack")
    //_manager->onAddRemoteTrack(this, transceiver.get());

    //    const char * pMid  = transceiver->mid()->c_str();
    //    int iMid = atoi(pMid);
    //    RTC_LOG(INFO)  << "OnAddTrack " <<  " mid "  << pMid;
    //    if(  transceiver->current_direction() !=  webrtc::RtpTransceiverDirection::kInactive &&
    //    transceiver->direction() !=  webrtc::RtpTransceiverDirection::kInactive  )
    //    {
    //
    //        rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track =
    //                transceiver->receiver()->track();
    //        RTC_LOG(INFO)  << "OnAddTrack " << track->id() <<  " kind " << track->kind() ;
    //
    //        if (track && remote_video_observer_[0] &&
    //            track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
    //            static_cast<webrtc::VideoTrackInterface*>(track.get())
    //                    ->AddOrUpdateSink(remote_video_observer_[0].get(), rtc::VideoSinkWants());
    //            RTC_LOG(LS_INFO) << "Remote video sink set up: " << track;
    //
    //        }
    //
    //    }
}

void Peer::OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver)
{
    LInfo(_peerid, ": OnRemoveTrack")
}

void Peer::OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream)
{
    LInfo(_peerid, ": OnRemoveStream") OnRemoveStream(stream.get());
}


void Peer::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> stream)
{
    LInfo(_peerid, ": OnDataChannel") assert(0 && "virtual");
}


void Peer::OnAddStream(webrtc::MediaStreamInterface *stream)
{
    // assert(_mode == Answer);

    LInfo(_peerid, ": On add stream") _manager->onAddRemoteStream(this, stream);
}


void Peer::OnRemoveStream(webrtc::MediaStreamInterface *stream)
{
    // assert(_mode == Answer);

    LInfo(_peerid, ": On remove stream") _manager->onRemoveRemoteStream(this, stream);
}


void Peer::OnIceCandidate(const webrtc::IceCandidateInterface *candidate)
{
    /*  std::string mid = candidate->sdp_mid() ;

      int line = candidate->sdp_mline_index();
     // Only for use internally.
      cricket::Candidate& can =  (cricket::Candidate& ) candidate->candidate();


      rtc::SocketAddress& add =  ( rtc::SocketAddress& ) can.address();

      bool x  =add.IsPrivateIP();
      if(x)
      add.SetIP("44.226.10.202");

     */

    std::string sdp;
    if (!candidate->ToString(&sdp))
    {
        LError(_peerid, ": Failed to serialize candidate") assert(0);
        return;
    }

    LDebug(_peerid, sdp);
    _manager->sendCandidate(this, candidate->sdp_mid(), candidate->sdp_mline_index(), sdp);
}


void Peer::OnSuccess(webrtc::SessionDescriptionInterface *desc)
{
    LDebug(_peerid, ": Set local description")

        cricket::SessionDescription *desc1
        = desc->description();


    /*for (const auto& content : desc1->contents()) {
       auto* transport_info = desc1->GetTransportInfoByName(content.name);
       transport_info->description.ice_mode = cricket::IceMode::ICEMODE_LITE;
      // transport_info->description.connection_role =  cricket::CONNECTIONROLE_ACTIVE;
       transport_info->description.transport_options.clear();
        transport_info->description.transport_options.push_back("renomination");

     }*/

    /*
    const rtc::SocketAddress kCallerAddress1("192.168.0.17", 1111);
    cricket::Candidate candidate1 = CreateLocalUdpCandidate(kCallerAddress1);
    AddCandidateToFirstTransport(&candidate1, SDP);
    const rtc::SocketAddress kCallerAddress2("192.168.0.17", 1001);
    cricket::Candidate candidate2 = CreateLocalTcpCandidate(kCallerAddress2);
    AddCandidateToFirstTransport(&candidate2, SDP);
    */

    _peerConnection->SetLocalDescription(DummySetSessionDescriptionObserver::Create(), desc);

    // Send an SDP offer to the peer

    // if(hasIceLiteOffer)
    {
        std::string sdp;
        if (!desc->ToString(&sdp))
        {
            LError(_peerid, ": Failed to serialize local sdp") assert(0);
            return;
        }
        _manager->sendSDP(this, desc->type(), sdp);
    }
}


void Peer::OnFailure(const std::string &error)
{
    LError(_peerid, ": On failure: ", error)

        _manager->onFailure(this, error);
}


void Peer::setPeerFactory(rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory)
{
    assert(!_context->factory);  // should not be already set via PeerManager
    _context->factory = factory;
}


std::string Peer::peerid() const
{
    return _peerid;
}

// void Peer::mute( const json& m)
// {
//     bool val = m.get<bool>();
//
//     SInfo << _peerid <<  ": On mute: " <<  val ;
//
//    std::vector<rtc::scoped_refptr<webrtc::RtpSenderInterface>> senders =  _peerConnection->GetSenders();
//
//    std::string trackid;
//
//    std::string cmd =  "pause";
//
//    for (const auto& sender : senders)
//    {
//        rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track = sender->track();
//        if( track && track->kind() == "video")
//        {
//           trackid =  track->id();  // removed prefixed vl
//           SInfo <<  track->kind() << " " <<  trackid ;
//           track->set_enabled(!val);
//
//        }
//
//    }
//
//
//    _manager->_capturer.oncommand( this, trackid , cmd, val, 0 );
//
// }



    

// webrtc::FakeConstraints& Peer::constraints()
// {
//     return _constraints;
// }


webrtc::PeerConnectionFactoryInterface *Peer::factory() const
{
    return _context->factory.get();
}


rtc::scoped_refptr<webrtc::PeerConnectionInterface> Peer::peerConnection() const
{
    return _peerConnection;
}


// rtc::scoped_refptr<webrtc::MediaStreamInterface> Peer::stream() const
//{
//     return _stream;
// }


//
// Dummy Set Peer Description Observer
//


void DummySetSessionDescriptionObserver::OnSuccess()
{
    LDebug("On SDP parse success")
}


void DummySetSessionDescriptionObserver::OnFailure(const std::string &error)
{
    LError("On SDP parse error: ", error)
    // assert(0);
}


}  // namespace web_rtc
}  // namespace base
