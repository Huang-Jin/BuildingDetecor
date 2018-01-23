// BuildingDetectorDemo.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <Windows.h>
#include <opencv2/opencv.hpp>
//#include "../BuildingDetecor/BuildingDetector.h"

using namespace cv;
using namespace std;

const Scalar RED = Scalar(0, 0, 255);
const Scalar PINK = Scalar(230, 130, 255);
const Scalar BLUE = Scalar(255, 0, 0);
const Scalar LIGHTBLUE = Scalar(255, 255, 160);
const Scalar GREEN = Scalar(0, 255, 0);

const int SHIFT_KEY = CV_EVENT_FLAG_SHIFTKEY; //Shift键

typedef cv::Mat(*BuildDetRec)(std::string, int, int, int, int);
typedef cv::Mat(*BuildDetPt)(std::string, int, int, int, int, std::vector<cv::Point>, std::vector<cv::Point>);

class Demo
{
public:
	enum{ NOT_SET = 0, IN_PROCESS = 1, SET = 2 };
	static const int m_radius = 2;
	static const int m_thickness = -1;

	void reset();
	void setImageAndWinName(const Mat& _image, const string& _winName);
	void showImage() const;
	void addPoints(Point p, bool isFgd);
	void mouseClick(int event, int x, int y, int flags, void* param);
	void clearPts();

	Rect getRect() { return m_rect; }
	Mat  getMask() { return m_mask; }
	void setMask(Mat mask) { m_mask = mask; }
	vector<cv::Point> getFgdPts() { return m_fgdPxls; }
	vector<cv::Point> getBgdPts() { return m_bgdPxls; }

private:
	const string* m_winName;
	const Mat* m_image;
	Mat m_mask;

	uchar m_rectState, m_lbState, m_rbState;
	bool m_isInitialized;

	Rect m_rect;
	vector<Point> m_fgdPxls, m_bgdPxls;
};

void Demo::reset()
{
	m_rectState = NOT_SET;
	m_lbState = NOT_SET;
	m_rbState = NOT_SET;
	
	if (!m_mask.empty())
	{
		m_mask.release();
	}

	m_bgdPxls.clear(); m_fgdPxls.clear();
}

void Demo::setImageAndWinName(const Mat& _image, const string& _winName)
{
	if (_image.empty() || _winName.empty())
		return;
	m_image = &_image;
	m_winName = &_winName;
	reset();
}

void Demo::showImage() const
{
	if (m_image->empty() || m_winName->empty())
		return;

	Mat res;
	if (!m_mask.empty())
	{
		m_image->copyTo(res, m_mask);
	}
	else
	{
		res = m_image->clone();
	}

	vector<Point>::const_iterator it;
	for (it = m_bgdPxls.begin(); it != m_bgdPxls.end(); ++it)
		circle(res, *it, m_radius, BLUE, m_thickness);
	for (it = m_fgdPxls.begin(); it != m_fgdPxls.end(); ++it)
		circle(res, *it, m_radius, RED, m_thickness);
	
	/*画矩形*/
	if (m_rectState == IN_PROCESS || m_rectState == SET)
		rectangle(res, Point(m_rect.x, m_rect.y), Point(m_rect.x + m_rect.width, m_rect.y + m_rect.height), GREEN, 2);

	imshow(*m_winName, res);
}

void Demo::addPoints(Point p, bool isFgd)
{
	vector<Point> *pxls;
	uchar value;
	if (isFgd) 
	{
		pxls = &m_fgdPxls;
		value = GC_FGD;    //1
	}
	else 
	{
		pxls = &m_bgdPxls;
		value = GC_BGD;    //1
	}

	pxls->push_back(p);
}

/*鼠标响应函数，参数flags为CV_EVENT_FLAG的组合*/
void Demo::mouseClick(int event, int x, int y, int flags, void*)
{
	switch (event)
	{
	case CV_EVENT_LBUTTONDOWN:
	{
		bool isShift = (flags & SHIFT_KEY) != 0;
		if (m_rectState == NOT_SET && !isShift)
		{
			m_rectState = IN_PROCESS; //表示正在画矩形
			m_rect = Rect(x, y, 1, 1);
		}
		if (isShift && m_rectState == SET) //按下了shift键，且画好了矩形，表示正在画前景点
			m_lbState = IN_PROCESS;
	}
		break;
	case CV_EVENT_LBUTTONUP:
		if (m_rectState == IN_PROCESS)
		{
			m_rect = Rect(Point(m_rect.x, m_rect.y), Point(x, y));   //矩形结束
			m_rectState = SET;
			showImage();
		}
		if (m_lbState == IN_PROCESS)
		{
			addPoints(Point(x, y), true);  
			m_lbState = SET;
			showImage();
		}
		break;
	case CV_EVENT_RBUTTONDOWN:
	{
		bool isShift = (flags & SHIFT_KEY) != 0;
		if (isShift && m_rectState == SET)	//按下了shift键，且画好了矩形，表示正在画背景点
			m_rbState = IN_PROCESS;
	}
		break;
	case CV_EVENT_RBUTTONUP:
		if (m_rbState == IN_PROCESS) 
		{
			addPoints(Point(x, y), false);    //画出背景点
			m_rbState = SET;
			showImage();
		}
		break;
	case CV_EVENT_MOUSEMOVE:
		if (m_rectState == IN_PROCESS)
		{
			m_rect = Rect(Point(m_rect.x, m_rect.y), Point(x, y));
			showImage();    //不断的显示图片
		}
		else if (m_lbState == IN_PROCESS)
		{
			addPoints(Point(x, y), true);
			showImage();
		}
		else if (m_rbState == IN_PROCESS)
		{
			addPoints(Point(x, y), false);
			showImage();
		}
		break;
	}
}

void Demo::clearPts()
{
	m_fgdPxls.clear();
	m_bgdPxls.clear();
}

Demo bdDemo;
void on_mouse(int event, int x, int y, int flags, void* param)
{
	bdDemo.mouseClick(event, x, y, flags, param);
}


void writeMask(string filename, Mat binMask)
{
	if (binMask.empty())
		return;
	imwrite(filename, binMask * 255);
}

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc != 2)
	{
		printf("Please input the image filename as parameter\r\n");
		return -1;
	}
	

	BuildDetPt build_detector;
	HINSTANCE hdll;
	hdll = LoadLibraryA("BuildingDetecor.dll");
	if (hdll == NULL)
	{
		FreeLibrary(hdll);
		return -1;
	}

	build_detector = (BuildDetPt)GetProcAddress(hdll, "building_detector_pt");
	if (build_detector == NULL)
	{
		FreeLibrary(hdll);
		return -1;
	}

	string filename(argv[1]);
	// "../images/9-65.tiff";
	Mat img = imread(filename);

	const string m_winName = "image";
	cvNamedWindow(m_winName.c_str(), CV_WINDOW_AUTOSIZE);
	cvSetMouseCallback(m_winName.c_str(), on_mouse, 0);

	bdDemo.setImageAndWinName(img, m_winName);
	bdDemo.showImage();

	Mat ret_mask;
	for (;;)
	{
		int c = cvWaitKey(0);
		switch ((char)c)
		{
		case '\x1b':
			cout << "Exiting ..." << endl;
			goto exit_main;
		case 'r':
			cout << endl;
			bdDemo.reset();
			bdDemo.showImage();
			break;
		case 'w':
			writeMask("p.png", bdDemo.getMask());
			break;
		case 'd':
			Rect rect = bdDemo.getRect();
			bdDemo.setMask(build_detector(filename, rect.x, rect.y, rect.width, rect.height, bdDemo.getFgdPts(), bdDemo.getBgdPts()));
			bdDemo.clearPts();
			bdDemo.showImage();
			break;
		}
	}

exit_main:
	cvDestroyWindow(m_winName.c_str());
	FreeLibrary(hdll);
	return 0;
}

