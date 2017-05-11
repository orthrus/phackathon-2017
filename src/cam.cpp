
#include "cam.h"
#include <iostream>
#include <thread>
#include <time.h>
#include <mutex>

CCam::CCam()
{

}

CCam::~CCam()
{
}

void CCam::Start()
{
  std::cout << "Opening camera" << std::endl;
  if(_capture.open(0))
  {
    std::cout << "Camera opened" << std::endl;

    _capture.set(CV_CAP_PROP_FRAME_WIDTH, 720);
    _capture.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
    if(_capture.set(CV_CAP_PROP_FPS, 60))
    {
      std::cout << "Set framerate" << std::endl;

      //cv::startWindowThread();

      //cv::namedWindow("thresh");
      std::cout << "Created window" << std::endl;
      _running = true;

      int i = 0, prevI = 0;

      cv::Mat diffFrame, threshFrame;


      long long prevTime = 0;
      long long curTime = 0;
      cv::Mat frame1;

      std::thread t = std::thread([=]() mutable {
        
        while(_running)
        {
          cv::Mat frame2, diffFrameTmp, threshFrameTmp;
          if(frame1.empty())
          {
            if(!_capture.read(frame1))
            {
              break;
            }
          }

          if(!_capture.read(frame2))
          {
            break;
          }

          cv::absdiff(frame1, frame2, diffFrameTmp);
          cv::threshold(diffFrameTmp, threshFrameTmp, 80, 255, cv::THRESH_BINARY);

          {
            std::lock_guard<std::mutex> lock(_mutex);
            if(!diffFrameTmp.empty())
            {
              diffFrame = diffFrameTmp.clone();
            }
            if(!threshFrameTmp.clone())
            {
              threshFrame = threshFrameTmp.clone();
            }
          }

          frame1 = frame2.clone();

          if((i++) % 60 == 0)
          {
            struct timespec spec;
            clock_gettime(CLOCK_REALTIME, &spec);
            curTime = (spec.tv_sec * 1000) + round(spec.tv_nsec / 1.0e6);
            if(prevTime != 0)
            {
              std::cout << "reading " << i - prevI << " frames took " << curTime - prevTime << "ms" << std::endl;
            }
            prevTime = curTime;
            prevI = i;
          }
          cv::waitKey(1);
        }

      });


      cv::namedWindow("diff");

      while(_running)
      {
        cv::Mat diff, thresh, frame;
        {
          std::lock_guard<std::mutex> lock(_mutex);
          if(!diffFrame.empty())
          {
            std::cout << "clone diff" << std::endl;
            diff = diffFrame.clone();
          }
          if(!threshFrame.empty())
          {
            std::cout << "clone thresh" << std::endl;
            thresh = threshFrame.clone();
          }
          frame = frame1.clone();
        }

        if(!diff.empty())
        {
          std::cout << "show frame" << std::endl;
          cv::imshow("diff", frame);
        }
        //std::cout << "waitkey1" << std::endl;
        cv::waitKey(1);
        //std::cout << "waitkey2" << std::endl;
      }



      std::cout << "Stopped reading" << std::endl;
      _running = false;
    }

    _capture.release();
  }
}
