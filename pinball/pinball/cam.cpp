
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

using namespace cv;
using namespace std;
void CCam::Start()
{
  std::cout << "Opening camera" << std::endl;
  if(_capture.open(0))
  {
    std::cout << "Camera opened" << std::endl;

    _capture.set(CV_CAP_PROP_FRAME_WIDTH, 480);
    _capture.set(CV_CAP_PROP_FRAME_HEIGHT, 320);
    if(_capture.set(CV_CAP_PROP_FPS, 60))
    {
      std::cout << "Set framerate" << std::endl;

      cv::namedWindow("frame");
      cv::Mat frame;
      bool first = true;

      cv::VideoWriter recorder;
      int ex = static_cast<int>(_capture.get(CV_CAP_PROP_FOURCC));     // Get Codec Type- Int form

      //recoder.open("/home/pi/video.avi", ex, 60, Size(480, 320), true);

      while(_capture.read(frame))
      {
        cv::imshow("frame", frame);
        if(first)
        {
          cv::imwrite("/home/pi/baseline.jpg", frame);
          first = false;
        }
        
        cv::waitKey(1);
      }

      //cv::startWindowThread();

      /*//cv::namedWindow("thresh");
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

          diffFrameTmp.addref();
          {
            std::lock_guard<std::mutex> lock(_mutex);
            if(!diffFrameTmp.empty())
            {
              diffFrame = diffFrameTmp.clone();
            }
            if(!threshFrameTmp.empty())
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
          cv::imshow("diff", diff);
        }
        //std::cout << "waitkey1" << std::endl;
        cv::waitKey(1);
        //std::cout << "waitkey2" << std::endl;
      }

*/

      std::cout << "Stopped reading" << std::endl;
      _running = false;
    }

    _capture.release();
  }
}
