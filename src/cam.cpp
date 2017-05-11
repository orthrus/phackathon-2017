
#include "cam.h"
#include <iostream>
#include <thread>
#include <time.h>

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
      cv::namedWindow("diff");
      cv::namedWindow("thresh");
      std::cout << "Created window" << std::endl;
      _running = true;

      int i = 0, prevI = 0;
      cv::Mat frame1;
      cv::Mat frame2;

      cv::Mat diffFrame;
      cv::Mat threshFrame;

      long long prevTime = 0;
      long long curTime = 0;;

      while(_running)
      {
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

        cv::absdiff(frame1, frame2, diffFrame);
        cv::threshold(diffFrame, threshFrame, 80, 255, cv::THRESH_BINARY);

        frame1 = frame2;

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

        cv::imgshow("diff", diffFrame);
        cv::imgshow("thresh", threshFrame);
        cv::waitKey(1);
      }

      std::cout << "Stopped reading" << std::endl;
      _running = false;
    }

    _capture.release();
  }
}
