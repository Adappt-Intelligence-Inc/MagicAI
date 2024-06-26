
#ifndef WebRTC_WEBRTC_UTIL_H
#    define WebRTC_WEBRTC_UTIL_H


#    include "webrtc/webrtc.h"

//#include "media/base/videocapturer.h"
//#include "media/engine/webrtcvideocapturerfactory.h"
#    include "modules/video_capture/video_capture_factory.h"


namespace base
{
namespace web_rtc
{


std::vector<std::string> getVideoCaptureDevices();

// std::unique_ptr<cricket::VideoCapturer> openWebRtcVideoCaptureDevice(const std::string& deviceName = "");


}  // namespace web_rtc
}  // namespace base


#endif  // WebRTC_WEBRTC_H


/// @\}
