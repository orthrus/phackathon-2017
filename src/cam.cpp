
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

    _capture.set(CV_CAP_PROP_FRAME_WIDTH, 1280);
    _capture.set(CV_CAP_PROP_FRAME_HEIGHT, 7200);
    if(_capture.set(CV_CAP_PROP_FPS, 60))
    {
      std::cout << "Set framerate" << std::endl;

      cv::namedWindow("frame");
      std::cout << "Created window" << std::endl;

      cv::Mat frame;
      while(_capture.read(frame))
      {
        std::cout << "read frame" << std::endl;
        cv::imshow("frame", frame);
        cv::waitKey(1);
      }


    }

    _capture.release();
  }
}
