
#ifndef __INCLUDE_CAM_CPP__
#define __INCLUDE_CAM_CPP__
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/video.hpp>


class CCam
{
public:
  CCam();
  ~CCam();

  void Start();

private:
  cv::VideoCapture _capture;
};

#endif
