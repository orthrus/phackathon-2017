
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
      cv::Mat frame1;
      cv::Mat frame2;

      cv::Mat diffFrame, diffFrameTmp;
      cv::Mat threshFrame, threshFrameTmp;

      long long prevTime = 0;
      long long curTime = 0;

      std::thread t = std::thread([=]() {
        cv::Mat diff;
        cv::Mat thresh;
        cv::namedWindow("diff");

        while(_running)
        {
          {
            std::lock_guard<std::mutex> lock(_mutex);
            if(!diffFrame.empty())
              diff = diffFrame.clone();
            if(!threshFrame.empty())
              thresh = threshFrame.clone();
          }

          if(!diff.empty())
          {
            std::cout << "show frame" << std::endl;
            cv::imshow("diff", diff);
          }
          cv::waitKey(1);
        }

      });

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

        cv::absdiff(frame1, frame2, diffFrameTmp);
        cv::threshold(diffFrame, threshFrameTmp, 80, 255, cv::THRESH_BINARY);

        {
          std::lock_guard<std::mutex> lock(_mutex);
          diffFrame = diffFrameTmp.clone();
          threshFrame = threshFrameTmp.clone();
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
      }

      std::cout << "Stopped reading" << std::endl;
      _running = false;
    }

    _capture.release();
  }
}
