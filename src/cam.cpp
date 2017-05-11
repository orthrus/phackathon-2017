
#include "cam.h"
#include <iostream>
#include <thread>

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

      int i = 0;
      cv::Mat frame;
      while (_capture.read(frame))
      {
        std::stringstream ss;
        cv::rectangle(frame, cv::Point(10, 2), cv::Point(100, 20), cv::Scalar(255, 255, 255), -1);
        ss << i++;
        std::string frameNumberString = ss.str();
        cv::putText(frame, frameNumberString.c_str(), cv::Point(15, 15), CV_FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 0, 0));

        //cv::imshow("frame", frame);
        //cv::waitKey(1);
      }

      std::cout << "Stopped reading" << std::endl;
      _running = false;
    }

    _capture.release();
  }
}
