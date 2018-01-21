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

typedef cv::Mat(*BuildDet)(std::string, int, int, int, int);

class BuildingDetector
{
public:
	enum{ NOT_SET = 0, IN_PROCESS = 1, SET = 2 };
	static const int m_radius = 2;
	static const int m_thickness = -1;

	void reset();
	void setImageAndWinName(const Mat& _image, const string& _winName);
	void showImage() const;
	void mouseClick(int event, int x, int y, int flags, void* param);

	Rect getRect() { return m_rect; }
	Mat  getMask() { return m_mask; }
	void setMask(Mat mask) { m_mask = mask; }

private:
	const string* m_winName;
	const Mat* m_image;
	Mat m_mask;
	Mat m_bgdModel, m_fgdModel;
	Mat m_preMask, m_preImage;

	uchar m_rectState, m_lblsState, m_prLblsState;
	bool m_isInitialized;

	Rect m_rect;
	vector<Point> m_fgdPxls, m_bgdPxls, m_prFgdPxls, m_prBgdPxls;
};

/*给类的变量赋值*/
void BuildingDetector::reset()
{
	m_rectState = NOT_SET;
	if (!m_mask.empty())
	{
		m_mask.release();
	}
}

/*给类的成员变量赋值*/
void BuildingDetector::setImageAndWinName(const Mat& _image, const string& _winName)
{
	if (_image.empty() || _winName.empty())
		return;
	m_image = &_image;
	m_winName = &_winName;
	reset();
}

/*显示4个点，一个矩形和图像内容，因为后面的步骤很多地方都要用到这个函数，所以单独拿出来*/
void BuildingDetector::showImage() const
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
	
	/*画矩形*/
	if (m_rectState == IN_PROCESS || m_rectState == SET)
		rectangle(res, Point(m_rect.x, m_rect.y), Point(m_rect.x + m_rect.width, m_rect.y + m_rect.height), GREEN, 2);

	imshow(*m_winName, res);
}

/*鼠标响应函数，参数flags为CV_EVENT_FLAG的组合*/
void BuildingDetector::mouseClick(int event, int x, int y, int flags, void*)
{
	switch (event)
	{
	case CV_EVENT_LBUTTONDOWN: // set m_rect or GC_BGD(GC_FGD) labels
	{
		m_rectState = IN_PROCESS; //表示正在画矩形
		m_rect = Rect(x, y, 1, 1);
	}
		break;
	case CV_EVENT_LBUTTONUP:
		if (m_rectState == IN_PROCESS)
		{
			m_rect = Rect(Point(m_rect.x, m_rect.y), Point(x, y));   //矩形结束
			m_rectState = SET;
			assert(m_bgdPxls.empty() && m_fgdPxls.empty() && m_prBgdPxls.empty() && m_prFgdPxls.empty());
			showImage();
		}
		break;
	case CV_EVENT_MOUSEMOVE:
		if (m_rectState == IN_PROCESS)
		{
			m_rect = Rect(Point(m_rect.x, m_rect.y), Point(x, y));
			assert(m_bgdPxls.empty() && m_fgdPxls.empty() && m_prBgdPxls.empty() && m_prFgdPxls.empty());
			showImage();    //不断的显示图片
		}
		break;
	}
}

BuildingDetector bdDemo;
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
	BuildDet build_detector;
	HINSTANCE hdll;
	hdll = LoadLibraryA("BuildingDetecor.dll");
	if (hdll == NULL)
	{
		FreeLibrary(hdll);
		return -1;
	}

	build_detector = (BuildDet)GetProcAddress(hdll, "building_detector");
	if (build_detector == NULL)
	{
		FreeLibrary(hdll);
		return -1;
	}

	string filename = "../images/7-48.tiff";
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
			bdDemo.setMask(build_detector(filename, rect.x, rect.y, rect.width, rect.height));
			bdDemo.showImage();
			break;
		}
	}

exit_main:
	cvDestroyWindow(m_winName.c_str());
	FreeLibrary(hdll);
	return 0;
}

