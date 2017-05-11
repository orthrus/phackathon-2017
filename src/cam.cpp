
#include "cam.h"
#include <iostream>

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

    if(_capture.set(CV_CAP_PROP_FPS, 60))
    {
      std::cout << "Set framerate" << std::endl;

      cv::namedWindow("frame");

      cv::Mat frame;
      while(capture.read(frame))
      {
        cv::imshow("frame", frame);
        cv::waitKey(1);
      }


    }

    _capture.release();
  }
}
