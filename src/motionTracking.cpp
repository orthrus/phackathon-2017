//motionTracking.cpp

//Written by  Kyle Hounslow, January 2014

//Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software")
//, to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
//and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

//The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
//IN THE SOFTWARE.

#include <iostream>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv/cv.h>
#include <functional>

#include <opencv2/highgui.hpp>

#include <ctime>
#ifdef __linux__
#include "serial.h"
#endif
#include <time.h>
//#include "Leap.h"

using namespace std;
using namespace cv;

#ifdef __linux__
CSerial ser;
#endif

//our sensitivity value to be used in the threshold() function
const static int SENSITIVITY_VALUE = 60;
//size of blur used to smooth the image to remove possible noise and
//increase the size of the object we are trying to track. (Much like dilate and erode)
const static int BLUR_SIZE = 15;
//we'll have just one object to search for
//and keep track of its position.
int theObject[2] = {0,0};
//bounding rectangle of the object, we will use the center of this as its position.
Rect objectBoundingRectangle = Rect(0,0,0,0);


//int to string helper function
string intToString(int number){

	//this function has a number input and string output
	std::stringstream ss;
	ss << number;
	return ss.str();
}

struct TPosition
{
	int x;
	int y;
};

#define HIS_SIZE 5
#define HIST_SIZE 50


vector<Point2f> leftRegion;
vector<Point2f> rightRegion;

double latency = 2.0;


class History
{
private:
	TPosition* positions;
	size_t size;
	uint pos = 0;
public:
	History(size_t size)
	{
		positions = new TPosition[size];
		this->size = size;
		Clear();
	}

	~History()
	{
		delete positions;
	}

	void Clear()
	{
		memset(positions, 0, sizeof(TPosition)*size);
		pos = 0;
	}

	TPosition* GetPositions()
	{
		return positions;
	}

	size_t GetSize()
	{
		return size;
	}

	void Set(int x, int y)
	{
		positions[pos].x = x;
		positions[pos].y = y;
		pos++;
		if (pos == size) pos = 0;
	}

	void Do(bool onlyLastTwo, const std::function< void(int,int,bool,bool) >& lambda)
	{
		for (size_t i = onlyLastTwo?size-2:0; i < size; i++) {
			int cp = (int)(pos + i);
			if (cp >= size) cp = cp%size;

			lambda(positions[cp].x, positions[cp].y, positions[cp].x!=0 || positions[cp].y!=0,i+1==size);
		}
	}
};

History* historyPositions;
History* historyHits;


void drawRegion(Mat img, vector<Point2f>& region, const Scalar& col)
{
	if (region.size() < 2) return;

	const Point2f* first = nullptr;
	const Point2f* current = nullptr;
	const Point2f* previous = nullptr;

	for (int i = 0; i < region.size(); i++)
	{
		current = &region[i];

		if (first == nullptr) {
			first = current;
		}

		if (previous != nullptr) {

			line(img, *previous, *current, col, 5);
		}

		previous = current;
	}

	line(img, *previous, *first, col, 5);
}

void Flip(int id)
{
	cout << "FLIP " << id << endl;

	switch(id)
	{
		case 1:
			ser.flipper(CSerial::BOTTOMLEFT, 100);
			break;
		case 2:
			ser.flipper(CSerial::BOTTOMRIGHT, 100);
			break;
		case 3:
			ser.flipper(CSerial::TOPRIGHT, 100);
			break;
	}
}


struct FlipTrigger
{
	int id;
	vector<Point2f> region;
};

FlipTrigger triggerFlipBottomLeft;
FlipTrigger triggerFlipBottomRight;
FlipTrigger triggerFlipTopRight;





Mat* ff;

double cross(Point v1, Point v2) {
	return v1.x*v2.y - v1.y*v2.x;
}
bool getIntersectionPoint(Point a1, Point a2, Point b1, Point b2) {
	Point p = a1;
	Point q = b1;
	Point r(a2 - a1);
	Point s(b2 - b1);

	if (cross(r, s) == 0) { return false; }

	double t = cross(q - p, s) / cross(r, s);


	line(*ff, a1, a2, Scalar(255, 0, 0));
	line(*ff, b1, b2, Scalar(255, 0, 0));

	return true;
}

bool pointonline(const Point& p, const Point& l1, const Point& l2)
{
	if (l1.x < l2.x)
	{
		if (l1.y < l2.y)
		{
			return p.x >= l1.x && p.x <= l2.x && p.y >= l1.y && p.y <= l2.y;
		}
		else
		{
			return p.x >= l1.x && p.x <= l2.x && p.y < l1.y && p.y > l2.y;
		}
	}
	else
	{
		if (l1.y < l2.y)
		{
			return p.x < l1.x && p.x > l2.x && p.y >= l1.y && p.y <= l2.y;
		}
		else
		{
			return p.x < l1.x && p.x > l2.x && p.y < l1.y && p.y > l2.y;
		}
	}
}

bool intersection(const Point2f& o1, const Point2f& p1, const Point2f& o2, const Point2f& p2)
{
	Point2f x = o2 - o1;
	Point2f d1 = p1 - o1;
	Point2f d2 = p2 - o2;

	float cross = d1.x*d2.y - d1.y*d2.x;
	if (abs(cross) < /*EPS*/1e-8)
		return false;

	//line(*ff, o1, p1,Scalar(255,0,0));
	//line(*ff, o2, p2, Scalar(255, 0, 0));

	Point2f r;
	double t1 = (x.x * d2.y - x.y * d2.x) / cross;
	r = o1 + d1 * t1;

	bool online =  pointonline(r,o1,p1) && pointonline(r,o2,p2);

	//if (online) circle(*ff, r, 10, Scalar(255, 0, 0));

	return online;
}

bool testTrigger(const Point& point1, const Point& point2, const FlipTrigger& flipTrigger) {

	if (flipTrigger.region.size() < 2) return false;

	const Point2f* first = nullptr;
	const Point2f* current = nullptr;
	const Point2f* previous = nullptr;

	for (int i = 0; i < flipTrigger.region.size(); i++)
	{
		current = &flipTrigger.region[i];

		if (first == nullptr) {
			first = current;
		}

		if (previous != nullptr) {

			if (intersection(point1, point2, *previous, *current))
			{ 
				return true;
			}
		}

		previous = current;
	}

	return intersection(point1, point2, *previous, *first);
}

void searchForMovement(bool debug, const Mat& input, Mat &cameraFeed){
	//notice how we use the '&' operator for the cameraFeed. This is because we wish
	//to take the values passed into the function and manipulate them, rather than just working with a copy.
	//eg. we draw to the cameraFeed in this function which is then displayed in the main() function.
	bool objectDetected=false;
	//Mat temp;
	//input.copyTo(temp);
	//these two vectors needed for output of findContours
	vector< vector<Point> > contours;
	vector<Vec4i> hierarchy;
	//find contours of filtered image using openCV findContours function
	//findContours(temp,contours,hierarchy,CV_RETR_CCOMP,CV_CHAIN_APPROX_SIMPLE );// retrieves all contours
	findContours(input,contours,hierarchy,CV_RETR_EXTERNAL,CV_CHAIN_APPROX_SIMPLE );// retrieves external contours

	//if contours vector is not empty, we have found some objects
	if(contours.size()>0)objectDetected=true;
	else objectDetected = false;

	ff = &cameraFeed;

	if(objectDetected){
		//the largest contour is found at the end of the contours vector
		//we will simply assume that the biggest contour is the object we are looking for.
		vector< vector<Point> > largestContourVec;
		largestContourVec.push_back(contours.at(contours.size()-1));
		//make a bounding rectangle around the largest contour then find its centroid
		//this will be the object's final estimated position.
		objectBoundingRectangle = boundingRect(largestContourVec.at(0));
		int xpos = objectBoundingRectangle.x+objectBoundingRectangle.width/2;
		int ypos = objectBoundingRectangle.y+objectBoundingRectangle.height/2;

		//update the objects positions by changing the 'theObject' array values
		theObject[0] = xpos , theObject[1] = ypos;
	}
	//make some temp x and y variables so we dont have to type out so much
	
	int x = theObject[0];
	int y = theObject[1];
	//draw some crosshairs on the object
	
	if (objectDetected) {
		if (debug) {
			circle(cameraFeed, Point(x, y), 20, Scalar(0, 255, 0), 2);
			line(cameraFeed, Point(x, y), Point(x, y - 25), Scalar(0, 255, 0), 2);
			line(cameraFeed, Point(x, y), Point(x, y + 25), Scalar(0, 255, 0), 2);
			line(cameraFeed, Point(x, y), Point(x - 25, y), Scalar(0, 255, 0), 2);
			line(cameraFeed, Point(x, y), Point(x + 25, y), Scalar(0, 255, 0), 2);
			putText(cameraFeed, "Tracking object at (" + intToString(x) + "," + intToString(y) + ")", Point(x, y), 1, 1, Scalar(255, 0, 0), 2);
		}
	}
	else {
		x = 0;
		y = 0;
	}

	historyPositions->Set(x, y);

	Point previous;
	historyPositions->Do(!debug,[debug, &previous,&cameraFeed](int x, int y, bool valid, bool last) {
		
		if (debug) {
			if (previous.x != 0 || previous.y != 0) {
				line(cameraFeed, previous, Point(x, y), Scalar(0, 255, 0), 2);
			}
		}

		if (last && valid && previous.x != 0 && previous.y != 0) {
			int dx = previous.x - x;
			int dy = previous.y - y;

			int nx = x - (int)(dx*latency);
			int ny = y - (int)(dy*latency);

			Point prediction(nx, ny);
			Point current(x, y);

			if (debug)
			{
				line(cameraFeed, prediction, current, Scalar(255, 0, 0), 2);
			}

			if (testTrigger(current, prediction, triggerFlipBottomLeft)) {
				Flip(triggerFlipBottomLeft.id);
				historyHits->Set(x, y);
				if (debug) drawRegion(cameraFeed, triggerFlipBottomLeft.region, Scalar(255, 0, 255));
			}

			if (testTrigger(current, prediction, triggerFlipBottomRight)) {
				Flip(triggerFlipBottomRight.id);
				historyHits->Set(x, y);
				if (debug) drawRegion(cameraFeed, triggerFlipBottomRight.region, Scalar(255, 0, 255));
			}

			/*if (testTrigger(current, prediction, triggerFlipTopRight)) {
				Flip(triggerFlipTopRight.id);
				historyHits->Set(x, y);
				if (debug) drawRegion(cameraFeed, triggerFlipTopRight.region, Scalar(255, 0, 255));
			}*/			

		}

		if (debug && valid) circle(cameraFeed, Point(x, y), 5, Scalar(0, 0, 255), 2);

		if (valid) {
			previous.x = x;
			previous.y = y;
		}
	
	});

	if (debug) {
		historyHits->Do(false, [&cameraFeed](int x, int y, bool valid, bool last) {
			if (valid) circle(cameraFeed, Point(x, y), 3, Scalar(255, 255, 255), 2);
		});
	}

}

void processFrame(Mat &capture, Mat& mask, int blur)
{
	if (mask.data) {
		cv::bitwise_and(capture, mask, capture);
	}
	cv::cvtColor(capture, capture, COLOR_BGR2GRAY);
	if (blur > 0) {
		cv::blur(capture, capture, cvSize(blur, blur));
	}
}

struct Setting
{
	char up;
	char down;
	int value;
	char* name;
	std::function< int(int, int) > valueCorrector;
	int limit_low;
	int limit_high;

	Setting(char u, char d, int v, char* n, int ll, int lh)
	{
		up = u;
		down = d;
		value = v;
		name = n;
		limit_low = ll;
		limit_high = lh;
	}

	void increase() { value++; if (value > limit_high) value = limit_high; }
	void decrease() { value--; if (value < limit_low) value = limit_low; }
};

std::vector<Setting*> settings;

long long getTimeMs()
{
	struct timespec spec;
	clock_gettime(CLOCK_REALTIME, &spec);
    return (spec.tv_sec * 1000) + round(spec.tv_nsec / 1.0e6);
}

int main(){
	#ifdef __linux__
	ser.init(false);
	ser.reset();
	#endif

	triggerFlipBottomLeft.id = 1;
	triggerFlipBottomLeft.region.push_back(Point2f(718, 288));
	triggerFlipBottomLeft.region.push_back(Point2f(680, 359));
	triggerFlipBottomLeft.region.push_back(Point2f(647, 288));

	triggerFlipBottomRight.id = 2;
	triggerFlipBottomRight.region.push_back(Point2f(680, 218));
	triggerFlipBottomRight.region.push_back(Point2f(718, 288));
	triggerFlipBottomRight.region.push_back(Point2f(647, 288));

	triggerFlipTopRight.id = 3;
	triggerFlipTopRight.region.push_back(Point2f(268, 102));
	triggerFlipTopRight.region.push_back(Point2f(327, 125));
	triggerFlipTopRight.region.push_back(Point2f(286, 169));

	historyPositions = new History(HIS_SIZE);
	historyHits = new History(HIST_SIZE);

	//some boolean variables for added functionality
	bool objectDetected = false;
	//these two can be toggled by pressing 'd' or 't'
	bool debugMode = true;
	bool trackingEnabled = true;
	//pause and resume code
	bool pause = false;
	//set up the matrices that we will need
	//the two frames we will be comparing
	Mat frame1, frame2, output, cap;
	//their grayscale images (needed for absdiff() function)
	Mat grayImage1,grayImage2;
	//resulting difference image
	Mat differenceImage;
	//thresholded difference image (for use in findContours() function)
	Mat thresholdImage;
	//video capture object.
	//VideoCapture capture;// (CV_CAP_DSHOW + 0);
	//sVideoCapture vcap( 0); 

	VideoCapture capture(0); // open the default camera
	if (!capture.isOpened())  // check if we succeeded
		return -1;

	capture.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
	capture.set(CV_CAP_PROP_FRAME_WIDTH, 720);
	capture.set(CV_CAP_PROP_FPS, 60);

	bool fromFile = false;


	int iblur = BLUR_SIZE/2;
	int isens = SENSITIVITY_VALUE;
	int iblur2 = BLUR_SIZE;
	int isens2 = SENSITIVITY_VALUE;
	
	int counter = 0;
	long long currTime = 0, prevTime = 0;

	cv::Mat *currentFrame = nullptr, *previousFrame = nullptr;

	while(1){
		//we can loop the video by re-opening the capture every time the video reaches its last frame

		if (fromFile) {
			//capture.open("C:\\bouncingBall.avi");
			capture.open("D:\\WIN_20170411_12_48_54_Pro.mp4");
			if (!capture.isOpened()) {
				cout << "ERROR ACQUIRING VIDEO FEED\n";
				getchar();
				return -1;
			}
		}


		//check if the video has reach its last frame.
		//we add '-1' because we are reading two frames from the video at a time.
		//if this is not included, we get a memory error!

		
		cv::Mat maskImage = cv::imread("mask.jpg");
		
		if (!maskImage.data) {
			capture.read(frame2);
			imwrite("masktemplate.jpg", frame2);
		}

		bool first = true;

		currentFrame = &frame1;
		previousFrame = &frame2;

		while(1){
		//while (capture.get(CV_CAP_PROP_POS_FRAMES)<capture.get(CV_CAP_PROP_FRAME_COUNT) - 1) {

			//read first frame
			if (frame1.empty())
			{
				capture.read(*previousFrame);

				//std::cout << "process previous" << std::endl;
				processFrame(*previousFrame, maskImage, iblur);

				first = false;
			}
			
			//copy second frame
			capture.read(*currentFrame);
			//currentFrame = &frame2;

			//std::cout << "process current" << std::endl;
			processFrame(*currentFrame, maskImage, iblur);

			//perform frame differencing with the sequential images. This will output an "intensity image"
			//do not confuse this with a threshold image, we will need to perform thresholding afterwards.
			//std::cout << "diff" << std::endl;
			cv::absdiff(*previousFrame, *currentFrame, differenceImage);
			//threshold intensity image at a given sensitivity value
			
			//std::cout << "threshold1" << std::endl;
			cv::threshold(differenceImage, thresholdImage, isens, 255, THRESH_BINARY);

			//use blur() to smooth the image, remove possible noise and
			//increase the size of the object we are trying to track. (Much like dilate and erode)
			
			//std::cout << "blur" << std::endl;
			cv::blur(thresholdImage, thresholdImage, cvSize(iblur2, iblur2));
			//threshold again to obtain binary image from blur output
			//std::cout << "threshold2" << std::endl;
			cv::threshold(thresholdImage, output, isens2, 255, THRESH_BINARY);

			//if tracking enabled, search for contours in our thresholded image
			if (trackingEnabled) {
				//std::cout << "search" << std::endl;
				searchForMovement(debugMode, output, thresholdImage);
				
				if (debugMode)
				{
					imshow("debug", thresholdImage);
				}
				else
				{
					cv::destroyWindow("debug");
				}

			}
			else {
				//show our captured frame
				

			}
			//check to see if a button has been pressed.
			//this 10ms delay is necessary for proper operation of this program
			//if removed, frames will not have enough time to referesh and a blank 
			//image will appear.

			
			switch(waitKey(debugMode?1:1)){

			case 27: //'esc' key has been pressed, exit program.
				return 0;
			case 116: //'t' has been pressed. this will toggle tracking
				trackingEnabled = !trackingEnabled;
				if(trackingEnabled == false) cout<<"Tracking disabled."<<endl;
				else cout<<"Tracking enabled."<<endl;
				break;
			case 100: //'d' has been pressed. this will debug mode
				debugMode = !debugMode;
				if(debugMode == false) cout<<"Debug mode disabled."<<endl;
				else cout<<"Debug mode enabled."<<endl;
				break;
			case 'b':
				if (iblur > 2) iblur--;
				break;
			case 'B':
				iblur++;
				break;
			case 's':
				if (isens > 2) isens--;
				break;
			case 'S':
				isens++;
				break;
			case 'n':
				iblur2--;
				break;
			case 'N':
				iblur2++;
				break;
			case 'a':
				isens2--;
				break;
			case 'A':
				isens2++;
				break;
			case 112: //'p' has been pressed. this will pause/resume the code.
				pause = !pause;
				if(pause == true){ cout<<"Code paused, press 'p' again to resume"<<endl;
				while (pause == true){
					//stay in this loop until 
					switch (waitKey()){
						//a switch statement inside a switch statement? Mind blown.
					case 112: 
						//change pause back to false
						pause = false;
						cout<<"Code resumed."<<endl;
						break;
					}
				}
				}


			}

			cv::Mat* tmp = currentFrame;
			currentFrame = previousFrame;
			previousFrame = tmp;
			//frame1 = grayImage2.clone();

			if((++counter) % 60 == 0)
			{
				currTime = getTimeMs();
				if(prevTime != 0)
				{
					std::cout << "reading " << counter << " frames took " << currTime - prevTime << "ms" << std::endl;
				}
				prevTime = currTime;
				counter = 0;
			}
		}
		 
		//release the capture before re-opening and looping again.
		capture.release();
	}


	delete historyHits;
	delete historyPositions;

	#ifdef __linux__
	ser.exit();
	#endif
	return 0;

}