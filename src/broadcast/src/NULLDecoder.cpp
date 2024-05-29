

#include "NULLDecoder.h"
#include "base/logger.h"

#include "tools.h"

extern "C" {
    //#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/parseutils.h>
}



#include "rtc_base/ref_counted_object.h"
#include "rtc_base/atomic_ops.h"
#include <chrono>
#include "base/platform.h"
#include "tools.h"
#include "common_video/h264/sps_parser.h"
#include "common_video/h264/h264_common.h"

#include "H264Framer.h"
#include "Settings.h"


#include <thread>

//////////////////////////////////////////////////////////////////////////

namespace base {
    namespace web_rtc {

        NULLDecoder::NULLDecoder(std::string  &cam): cam(cam)
        {
            
            SInfo << "NULLDecoder()";
            
            resetTimer();

            qframe = new stFrame();
            

            basicframe.media_type  = AVMEDIA_TYPE_VIDEO;
            basicframe.codec_id   = AV_CODEC_ID_H264;
            basicframe.stream_index  = 0;
            
            std::string tmp = Settings::configuration.storage +  "/"   ;
                  

            mf.load(tmp + "manifest.js");

            if( mf.root.is_null() )
            {
               mf.root = json::array();
            }
                    
     
        }

        NULLDecoder::~NULLDecoder() {
            
            delete qframe;
            SInfo << "~NULLDecoder()";
        }

        
        void NULLDecoder::WriteTofile( unsigned char *buf , int size)
        {
           
            char filePath[128];
             
            sprintf(filePath, "%s/frame-%.4d.h264", PathDate.c_str()   , ++frameCount);

                            
            in_file = fopen(filePath.c_str(),"wb");
            if(!in_file){
                   SError << "can't open file! " <<  in_file;
            }

            fwrite((const char*) muxframe->payload.data(), 1, meta->size, in_file);
            
            flose(in_file);
            
        }
 

        void NULLDecoder::runNULLEnc(unsigned char *buffer, int size, AVPictureType pict_type, int & recording ) 
        {

            bool idr = false;

            //bool slice = false;
            int cfg = 0;



            std::vector<webrtc::H264::NaluIndex> naluIndexes = webrtc::H264::FindNaluIndices( buffer, size);

            for (webrtc::H264::NaluIndex index : naluIndexes) {
                webrtc::H264::NaluType nalu_type = webrtc::H264::ParseNaluType(buffer[index.payload_start_offset]);
                // SInfo << __FUNCTION__ << " nalu:" << nalu_type << " payload_start_offset:" << index.payload_start_offset << " start_offset:" << index.start_offset << " size:" << index.payload_size;
                if (nalu_type == webrtc::H264::NaluType::kSps && cfg < 2) {

                    m_sps.resize(index.payload_size + index.payload_start_offset - index.start_offset);

                    memcpy(&m_sps[0], &buffer[index.start_offset], index.payload_size + index.payload_start_offset - index.start_offset);

//                           unsigned char *tmp = m_sps.data() + 4;
                    qframe->pushsps(m_sps);
                    unsigned num_units_in_tick, time_scale;
                    
    

                    H264Framer obj;
                    obj.analyze_seq_parameter_set_data(&buffer[index.payload_start_offset], index.payload_size, num_units_in_tick, time_scale);

                    //SInfo <<  " Got SPS fps "  << obj.fps << " width "  << obj.width  <<  " height " << obj.height ;

                    //absl::optional<webrtc::SpsParser::SpsState> sps_;
                    //static_cast<bool> (sps_ = webrtc::SpsParser::ParseSps(&buffer[index.start_offset + index.payload_start_offset + webrtc::H264::kNaluTypeSize], index.payload_size - webrtc::H264::kNaluTypeSize));

                    //width = sps_->width;
                   // height = sps_->height;

                    width = obj.width;
                    height = obj.height;

                    fps = obj.fps ? obj.fps:30;
                    vdelay = uint64_t(1000000) / uint64_t(fps); // default

                    
                    basicframe.width = width;
                    basicframe.height = height;
                    basicframe.fps = fps;
                    
                    
                    basicframe.payload.resize(m_sps.size());
                    basicframe.payload.insert(basicframe.payload.begin(), m_sps.begin(), m_sps.end());
                    basicframe.fillPars();
                    cb_mp4(&basicframe, false); 
                    
                    
                    cfg++;
                } else if (nalu_type == webrtc::H264::NaluType::kPps && cfg < 2) {

                    m_pps.resize(index.payload_size + index.payload_start_offset - index.start_offset);
                    // m_pps = webrtc::EncodedImageBuffer::Create((uint8_t*) & buffer[index.start_offset], index.payload_size + index.payload_start_offset - index.start_offset);
                    memcpy(&m_pps[0], &buffer[index.start_offset], index.payload_size + index.payload_start_offset - index.start_offset);
                    qframe->pushpps(m_pps);
                    
                    basicframe.payload.resize(m_pps.size());
                    basicframe.payload.insert(basicframe.payload.begin(), m_pps.begin(), m_pps.end());
                    basicframe.fillPars(); 
                    cb_mp4(&basicframe, false); 
                            
                    cfg++;
                } else if (nalu_type == webrtc::H264::NaluType::kIdr) {
                    idr = true;
                    SDebug << " idr:" << idr << " cfg:" << cfg << "  sps " << m_sps.size() << " pps  " << m_pps.size() << " vframecount " << vframecount << " width " << width << " height  " << height << " fps " << fps;
                    
                    basicframe.payload.resize(index.payload_size + index.payload_start_offset - index.start_offset);
                    basicframe.payload.insert(basicframe.payload.begin(),  &buffer[index.start_offset],   &buffer[index.start_offset] + index.payload_size + index.payload_start_offset - index.start_offset);
                    basicframe.fillPars();
                    cb_mp4(&basicframe, true); 
                    
                } else if (nalu_type == webrtc::H264::NaluType::kSlice) {
                    //slice = true;
                    
                    basicframe.payload.resize(index.payload_size + index.payload_start_offset - index.start_offset);
                    basicframe.payload.insert(basicframe.payload.begin(),  &buffer[index.start_offset],   &buffer[index.start_offset] + index.payload_size + index.payload_start_offset - index.start_offset);
                    basicframe.fillPars();
                    cb_mp4(&basicframe, false); 
          
                }
            }// end for
                    // 

                    //                rtc::scoped_refptr<webrtc::EncodedImageBufferInterface> encodedData = webrtc::EncodedImageBuffer::Create((uint8_t*) buffer, frameSize);
                    //                delete [] buffer;
                    //            // add last SPS/PPS if not present before an IDR
                    //            if (idr && (m_sps.size() != 0) && (m_pps.size() != 0)) {
                    //                //char * newBuffer = new char[frameSize + m_sps.size() + m_pps.size()];
                    //                // memcpy(newBuffer, &m_sps[0], m_sps.size());
                    //                // memcpy(newBuffer + m_sps.size(), &m_pps[0], m_pps.size());
                    //                //memcpy(newBuffer + m_sps.size() + m_pps.size(), buffer, frameSize);
                    //                // encodedData = webrtc::EncodedImageBuffer::Create((uint8_t*) newBuffer, encodedData->size() + m_sps->size() + m_pps->size());
                    //                // delete [] newBuffer;
                    //                // if(!frameNo)
                    //                {
                    //                    //std::cout << " payload size " <<   basic_frame->payload.size()  << std::endl << std::flush;
                    //
                    //                   // basic_frame->payload.insert(basic_frame->payload.begin(), m_pps.begin(), m_pps.end());
                    //
                    //                    //  std::cout << " payload size " <<   basic_frame->payload.size()  << std::endl << std::flush;
                    //
                    //                   // basic_frame->payload.insert(basic_frame->payload.begin(), m_sps.begin(), m_sps.end());
                    //
                    //                    frameNo++;
                    //
                    //                }
                    //
                    //
                    //            } else if (slice && frameNo) {
                    //                frameNo++;
                    //            } 
                    //            else
                    //            {
                    //              
                    //            }
                    
                    
                 
                    
            if( pict_type == AV_PICTURE_TYPE_I  )
            {
               // SInfo << "  AV_PICTURE_TYPE_I " ;
               qframe->clear();
            }
            
            
           // if (  pict_type == AV_PICTURE_TYPE_I)
            {    
                Store *store = new Store(&buffer[0], size, width, height, vframecount, idr);

                qframe->push(store);


                cb_frame(qframe); 


            }
            
            if( recording == 1 )
            {
                
                Timestamp ts;

                Timestamp::TimeVal time = ts.epochMicroseconds();

                int milli = int(time % 1000000) / 1000;


                std::time_t time1 = ts.epochTime();
                struct std::tm* tms = std::localtime(&time1);


                char date[100] = {'\0'}; //"%Y-%m-%d-%H-%M-%S"
                int len = std::strftime(date, sizeof (date), "%Y-%m-%d-%H-%M-%S", tms);

                if( dayDate != date)
                {   dayDate = date;
                    PathDate = Settings::configuration.storage +  "/" + dayDate  ;
                    if (!base::fs::exists(tmp ))
                    {
                       mkdir(PathDate.c_str(),0777);
                    }



                    mf.load(tmp + "manifest.js");

                    if( mf.root.is_null() )
                    {
                       mf.root = json::array();
                    }
                    
                    
                    mf.root.push_back(dayDate);
                   
                    

                    //mf.root["fps"]= muxframe->fps;
                }
                    
                
            }

            delayFrame();

            return;
        }
        
        
        
        void NULLDecoder::delayFrame( )
        {
            
           int speed =1; 
           
//           if( trackInfo.speed != 0)
//           {
//               speed = trackInfo.speed;
//           }


//           if(speed > 1 && speed < 4 )
//           {
//              vdelay =  vdelay/speed;
//           }
//           else if( speed <  -1)
//           {
//               vdelay =  vdelay*(-1*speed);
//           }
//        
//             

            ++vframecount;


            uint64_t deltamicro = CurrentTime_microseconds() - startStreaming;

        //    if(  deltamicro >=  vframecount * vdelay)
        //    {
        //         std::this_thread::sleep_for(std::chrono::microseconds(1004));
        //        SError <<  "deltamicro > vframecount * vdelay " << deltamicro - vframecount * vdelay;
        //    }
//#ifdef LOCALTEST
            while(deltamicro < vframecount * vdelay)
            {
                std::this_thread::sleep_for(std::chrono::microseconds(20000));
                deltamicro = CurrentTime_microseconds() - startStreaming;
            }
//#endif
        }
        
        void NULLDecoder::resetTimer()
        {
           startStreaming = CurrentTime_microseconds();
           vframecount =0;
        }




    }// ns web_rtc
}//ns base
