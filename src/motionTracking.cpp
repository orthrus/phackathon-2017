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

using namespace std;
using namespace cv;

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

double latency = 1.0;


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


void fillRegion(Mat img, vector<Point2f>& region, Scalar& col)
{
	/** Create some points */
	Point rook_points[1][4];
	rook_points[0][0] = region[0];
	rook_points[0][1] = region[1];
	rook_points[0][2] = region[2];
	rook_points[0][3] = region[3];


	const Point* ppt[1] = { rook_points[0] };
	int npt[] = { 4 };

	fillPoly(img,
		ppt,
		npt,
		1,
		col,
		8);
}

void flipLeft()
{
	cout << "FLIP LEFT\n";
}

void flipRight()
{
	cout << "FLIP RIGHT\n";
}

void searchForMovement(bool debug, Mat input, Mat &cameraFeed){
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

			if (debug)
			{
				line(cameraFeed, Point(nx, ny), Point(x, y), Scalar(255, 0, 0), 2);
			}

			bool left = (pointPolygonTest(leftRegion, Point2f(nx, ny), false) >= 0.0) || (pointPolygonTest(leftRegion, Point2f(x, y), false) >= 0.0);
			bool right = (pointPolygonTest(rightRegion, Point2f(nx, ny), false) >= 0.0) || (pointPolygonTest(rightRegion, Point2f(x, y), false) >= 0.0);

			if (!left && !right && nx != 0 && ny != 0 && dx != 0)
			{

				double a = dy / (double)dx;
				double b = ny - (a*nx);

				int flipx = (leftRegion[0].y - b) / a;


				if (ny < leftRegion[0].y)
				{
					if (flipx < rightRegion[0].x)
					{
						left = true;
					}
					else
					{
						right = true;
					}
				}
			}

			if (debug) {
				if (left) {
					fillRegion(cameraFeed, leftRegion, Scalar(255, 0, 255));
				}

				if (right) {
					fillRegion(cameraFeed, rightRegion, Scalar(255, 0, 255));
				}
			}

			if (left || right)
			{
				historyHits->Set(x, y);
			}

			if (left) flipLeft();
			if (right) flipRight();
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

void processFrame(Mat &capture, Mat mask, int blur)
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

int main(){

	leftRegion.push_back(Point2f(646, 139));
	leftRegion.push_back(Point2f(755, 139));
	leftRegion.push_back(Point2f(755, 160));
	leftRegion.push_back(Point2f(730, 160));


	rightRegion.push_back(Point2f(756, 139));
	rightRegion.push_back(Point2f(866, 139));
	rightRegion.push_back(Point2f(777, 160));
	rightRegion.push_back(Point2f(756, 160));


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
	//VideoCapture vcap( 0);

	VideoCapture capture(0); // open the default camera
	if (!capture.isOpened())  // check if we succeeded
		return -1;

	capture.set(CV_CAP_PROP_FRAME_HEIGHT, 720);
	capture.set(CV_CAP_PROP_FRAME_WIDTH, 1280);

	bool fromFile = false;


	int iblur = BLUR_SIZE/2;
	int isens = SENSITIVITY_VALUE;
	int iblur2 = BLUR_SIZE;
	int isens2 = SENSITIVITY_VALUE;
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


		cv::Mat maskImage = cv::imread("mask2.jpg");

		bool first = true;

		while(1){
		//while (capture.get(CV_CAP_PROP_POS_FRAMES)<capture.get(CV_CAP_PROP_FRAME_COUNT) - 1) {

			//read first frame
			if (frame1.empty()) {
				capture.read(frame1);

				processFrame(frame1,maskImage, iblur);

				first = false;
			}
			//copy second frame
			capture.read(frame2);

			processFrame(frame2, maskImage, iblur);

			//perform frame differencing with the sequential images. This will output an "intensity image"
			//do not confuse this with a threshold image, we will need to perform thresholding afterwards.
			cv::absdiff(frame1, frame2, differenceImage);
			//threshold intensity image at a given sensitivity value
			cv::threshold(differenceImage, thresholdImage, isens, 255, THRESH_BINARY);

			//use blur() to smooth the image, remove possible noise and
			//increase the size of the object we are trying to track. (Much like dilate and erode)
			cv::blur(thresholdImage, thresholdImage, cvSize(iblur2, iblur2));
			//threshold again to obtain binary image from blur output
			cv::threshold(thresholdImage, output, isens2, 255, THRESH_BINARY);

			//if tracking enabled, search for contours in our thresholded image
			if (trackingEnabled) {

				searchForMovement(debugMode, output,frame2);

				if (debugMode)
				{
					imshow("debug", frame2);
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
			switch(waitKey(debugMode?1:0)){

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

			frame1 = grayImage2.clone();
		}

		//release the capture before re-opening and looping again.
		capture.release();
	}


	delete historyHits;
	delete historyPositions;

	return 0;

}
