
#include "livethread.h"
//#include "logging.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <iterator>
#include "base/logger.h"
#include "base/platform.h"
#include "Settings.h"
// #define RECONNECT_VERBOSE   // by default, disable
// #define LIVE_SIGNAL_FRAMES // experimental




namespace base {
namespace web_rtc {
    
    LiveThread::LiveThread(const char* name, LiveConnectionContext& ctx):ctx(ctx)
    {
        
    }
    
    void LiveThread::run(){
        
       
    
        
        std::string filepath = "/var/tmp/test.264";
        
        FILE *fp = fopen(filepath.c_str(), "rb");

        if(!fp) {
          printf("Error: cannot open: %s\n", filepath.c_str());
          return ;
        }

        while(!stopped() )
        {
            int bytes_read = (int)fread(inbuf, 1, H264_INBUF_SIZE, fp);

            if(bytes_read) {
               basicframe.data = inbuf ;
               basicframe.sz = bytes_read;
            }
            else
            {

                  if(feof(fp))
                  {

                       if (fseek(fp, 0, SEEK_SET))
                           continue;

                  }

            }



           

           // ctx.muRecFrame.lock();
            if(ctx.liveFrame)
            ctx.liveFrame->run(&basicframe); // starts the frame filter chain
            //ctx.muRecFrame.unlock(); 

        
            basicframe.payload.resize(basicframe.payload.capacity());
       
             base::sleep(10);   
        }
        
        fclose(fp);
        
        
    }



}
}
