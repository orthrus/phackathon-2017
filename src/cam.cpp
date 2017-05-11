
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
      cv::namedWindow("frame");
      std::cout << "Created window" << std::endl;
      _running = true;

      int i = 0, prevI = 0;
      cv::Mat frame;

      long long prevTime = 0;
      long long curTime = 0;;
      while (_capture.read(frame))
      {
        //std::cout << "Frame " << i << std::endl;
        std::stringstream ss;
        cv::rectangle(frame, cv::Point(10, 2), cv::Point(100, 20), cv::Scalar(255, 255, 255), -1);
        ss << i++;
        std::string frameNumberString = ss.str();
        cv::putText(frame, frameNumberString.c_str(), cv::Point(15, 15), CV_FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 0, 0));

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
      }

      std::cout << "Stopped reading" << std::endl;
      _running = false;
    }

    _capture.release();
  }
}
