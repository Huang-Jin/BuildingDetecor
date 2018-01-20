// BuildingDetectorDemo.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

const Scalar RED = Scalar(0, 0, 255);
const Scalar PINK = Scalar(230, 130, 255);
const Scalar BLUE = Scalar(255, 0, 0);
const Scalar LIGHTBLUE = Scalar(255, 255, 160);
const Scalar GREEN = Scalar(0, 255, 0);

const int BGD_KEY = CV_EVENT_FLAG_CTRLKEY;  //Ctrl��
const int FGD_KEY = CV_EVENT_FLAG_SHIFTKEY; //Shift��

static void help()
{
	cout << "\nThis program demonstrates GrabCut segmentation -- select an object in a region\n"
		"and then grabcut will attempt to segment it out.\n"
		"Call:\n"
		"./grabcut <image_name>\n"
		"\nSelect a rectangular area around the object you want to segment\n" <<
		"\nHot keys: \n"
		"\tESC - quit the program\n"
		"\tr - restore the original m_image\n"
		"\tn - next iteration\n"
		"\n"
		"\tleft mouse button - set rectangle\n"
		"\n"
		"\tCTRL+left mouse button - set GC_BGD pixels\n"
		"\tSHIFT+left mouse button - set CG_FGD pixels\n"
		"\n"
		"\tCTRL+right mouse button - set GC_PR_BGD pixels\n"
		"\tSHIFT+right mouse button - set CG_PR_FGD pixels\n" << endl;
}

static void getBinMask(const Mat& comMask, Mat& binMask)
{
	if (comMask.empty() || comMask.type() != CV_8UC1)
		CV_Error(CV_StsBadArg, "comMask is empty or has incorrect type (not CV_8UC1)");
	if (binMask.empty() || binMask.rows != comMask.rows || binMask.cols != comMask.cols)
		binMask.create(comMask.size(), CV_8UC1);
	binMask = comMask & 1;  //�õ�m_mask�����λ,ʵ������ֻ����ȷ���Ļ����п��ܵ�ǰ���㵱��m_mask
}

class BuildingDetector
{
public:
	enum{ NOT_SET = 0, IN_PROCESS = 1, SET = 2 };
	static const int m_radius = 2;
	static const int m_thickness = -1;

	void reset();
	void back();
	void setImageAndWinName(const Mat& _image, const string& _winName);
	void showImage() const;
	void mouseClick(int event, int x, int y, int flags, void* param);
	int nextIter();
	int getIterCount() const { return m_iterCount; }

private:
	void setRectInMask();
	void setLblsInMask(int flags, Point p, bool isPr);

	const string* m_winName;
	const Mat* m_image;
	Mat m_mask;
	Mat m_bgdModel, m_fgdModel;
	Mat m_preMask, m_preImage;

	uchar m_rectState, m_lblsState, m_prLblsState;
	bool m_isInitialized;

	Rect m_rect;
	vector<Point> m_fgdPxls, m_bgdPxls, m_prFgdPxls, m_prBgdPxls;
	int m_iterCount;
};

/*����ı�����ֵ*/
void BuildingDetector::reset()
{
	if (!m_mask.empty())
		m_mask.setTo(Scalar::all(GC_BGD));
	m_bgdPxls.clear(); m_fgdPxls.clear();
	m_prBgdPxls.clear();  m_prFgdPxls.clear();

	m_isInitialized = false;
	m_rectState = NOT_SET;    //NOT_SET == 0
	m_lblsState = NOT_SET;
	m_prLblsState = NOT_SET;
	m_iterCount = 0;
}

void BuildingDetector::back()
{

}

/*����ĳ�Ա������ֵ*/
void BuildingDetector::setImageAndWinName(const Mat& _image, const string& _winName)
{
	if (_image.empty() || _winName.empty())
		return;
	m_image = &_image;
	m_winName = &_winName;
	m_mask.create(m_image->size(), CV_8UC1);
	reset();
}

/*��ʾ4���㣬һ�����κ�ͼ�����ݣ���Ϊ����Ĳ���ܶ�ط���Ҫ�õ�������������Ե����ó���*/
void BuildingDetector::showImage() const
{
	if (m_image->empty() || m_winName->empty())
		return;

	Mat res;
	Mat binMask;
	if (!m_isInitialized)
		m_image->copyTo(res);
	else
	{
		getBinMask(m_mask, binMask);
		m_image->copyTo(res, binMask);  //�������λ��0����1�����ƣ�ֻ������ǰ���йص�ͼ�񣬱���˵���ܵ�ǰ�������ܵı���
	}

	vector<Point>::const_iterator it;
	/*����4������ǽ�ѡ�е�4�ֵ��ò�ͬ����ɫ��ʾ����*/
	for (it = m_bgdPxls.begin(); it != m_bgdPxls.end(); ++it)  //���������Կ�����һ��ָ��
		circle(res, *it, m_radius, BLUE, m_thickness);
	for (it = m_fgdPxls.begin(); it != m_fgdPxls.end(); ++it)  //ȷ����ǰ���ú�ɫ��ʾ
		circle(res, *it, m_radius, RED, m_thickness);
	for (it = m_prBgdPxls.begin(); it != m_prBgdPxls.end(); ++it)
		circle(res, *it, m_radius, LIGHTBLUE, m_thickness);
	for (it = m_prFgdPxls.begin(); it != m_prFgdPxls.end(); ++it)
		circle(res, *it, m_radius, PINK, m_thickness);

	/*������*/
	if (m_rectState == IN_PROCESS || m_rectState == SET)
		rectangle(res, Point(m_rect.x, m_rect.y), Point(m_rect.x + m_rect.width, m_rect.y + m_rect.height), GREEN, 2);

	imshow(*m_winName, res);
}

/*�ò�����ɺ�m_maskͼ����m_rect�ڲ���3������ȫ��0*/
void BuildingDetector::setRectInMask()
{
	assert(!m_mask.empty());
	m_mask.setTo(GC_BGD);   //GC_BGD == 0
	m_rect.x = max(0, m_rect.x);
	m_rect.y = max(0, m_rect.y);
	m_rect.width = min(m_rect.width, m_image->cols - m_rect.x);
	m_rect.height = min(m_rect.height, m_image->rows - m_rect.y);
	(m_mask(m_rect)).setTo(Scalar(GC_PR_FGD));    //GC_PR_FGD == 3�������ڲ�,Ϊ���ܵ�ǰ����
}

void BuildingDetector::setLblsInMask(int flags, Point p, bool isPr)
{
	vector<Point> *bpxls, *fpxls;
	uchar bvalue, fvalue;
	if (!isPr) //ȷ���ĵ�
	{
		bpxls = &m_bgdPxls;
		fpxls = &m_fgdPxls;
		bvalue = GC_BGD;    //0
		fvalue = GC_FGD;    //1
	}
	else    //���ʵ�
	{
		bpxls = &m_prBgdPxls;
		fpxls = &m_prFgdPxls;
		bvalue = GC_PR_BGD; //2
		fvalue = GC_PR_FGD; //3
	}
	if (flags & BGD_KEY)
	{
		bpxls->push_back(p);
		circle(m_mask, p, m_radius, bvalue, m_thickness);   // thickness=-1�����Բ
	}
	if (flags & FGD_KEY)
	{
		fpxls->push_back(p);
		circle(m_mask, p, m_radius, fvalue, m_thickness);
	}
}

/*�����Ӧ����������flagsΪCV_EVENT_FLAG�����*/
void BuildingDetector::mouseClick(int event, int x, int y, int flags, void*)
{
	// TODO add bad args check
	switch (event)
	{
	case CV_EVENT_LBUTTONDOWN: // set m_rect or GC_BGD(GC_FGD) labels
	{
		bool isb = (flags & BGD_KEY) != 0,
			isf = (flags & FGD_KEY) != 0;
		if (m_rectState == NOT_SET && !isb && !isf)//ֻ���������ʱ
		{
			m_rectState = IN_PROCESS; //��ʾ���ڻ�����
			m_rect = Rect(x, y, 1, 1);
		}
		if ((isb || isf) && m_rectState == SET) //������alt������shift�����һ����˾��Σ���ʾ���ڻ�ǰ��������
			m_lblsState = IN_PROCESS;
	}
		break;
	case CV_EVENT_RBUTTONDOWN: // set GC_PR_BGD(GC_PR_FGD) labels
	{
		bool isb = (flags & BGD_KEY) != 0,
			isf = (flags & FGD_KEY) != 0;
		if ((isb || isf) && m_rectState == SET) //���ڻ����ܵ�ǰ��������
			m_prLblsState = IN_PROCESS;
	}
		break;
	case CV_EVENT_LBUTTONUP:
		if (m_rectState == IN_PROCESS)
		{
			m_rect = Rect(Point(m_rect.x, m_rect.y), Point(x, y));   //���ν���
			m_rectState = SET;
			setRectInMask();
			assert(m_bgdPxls.empty() && m_fgdPxls.empty() && m_prBgdPxls.empty() && m_prFgdPxls.empty());
			showImage();
		}
		if (m_lblsState == IN_PROCESS)   //�ѻ���ǰ�󾰵�
		{
			setLblsInMask(flags, Point(x, y), false);    //����ǰ����
			m_lblsState = SET;
			showImage();
		}
		break;
	case CV_EVENT_RBUTTONUP:
		if (m_prLblsState == IN_PROCESS)
		{
			setLblsInMask(flags, Point(x, y), true); //����������
			m_prLblsState = SET;
			showImage();
		}
		break;
	case CV_EVENT_MOUSEMOVE:
		if (m_rectState == IN_PROCESS)
		{
			m_rect = Rect(Point(m_rect.x, m_rect.y), Point(x, y));
			assert(m_bgdPxls.empty() && m_fgdPxls.empty() && m_prBgdPxls.empty() && m_prFgdPxls.empty());
			showImage();    //���ϵ���ʾͼƬ
		}
		else if (m_lblsState == IN_PROCESS)
		{
			setLblsInMask(flags, Point(x, y), false);
			showImage();
		}
		else if (m_prLblsState == IN_PROCESS)
		{
			setLblsInMask(flags, Point(x, y), true);
			showImage();
		}
		break;
	}
}

/*�ú�������grabcut�㷨�����ҷ����㷨���е����Ĵ���*/
int BuildingDetector::nextIter()
{
	if (m_isInitialized)
		//ʹ��grab�㷨����һ�ε���������2Ϊm_mask��������m_maskλ�ǣ������ڲ�������Щ�����Ǳ��������Ѿ�ȷ���Ǳ���������еĵ㣬��m_maskͬʱҲΪ���
		//������Ƿָ���ǰ��ͼ��
		grabCut(*m_image, m_mask, m_rect, m_bgdModel, m_fgdModel, 1);
	else
	{
		if (m_rectState != SET)
			return m_iterCount;

		if (m_lblsState == SET || m_prLblsState == SET)
			grabCut(*m_image, m_mask, m_rect, m_bgdModel, m_fgdModel, 1, GC_INIT_WITH_MASK);
		else if (m_rect.width == 0 || m_rect.height == 0)
		{
			return m_iterCount;
		}
		else
		{
			grabCut(*m_image, m_mask, m_rect, m_bgdModel, m_fgdModel, 1, GC_INIT_WITH_RECT);
		}
		
		m_isInitialized = true;
	}
	m_iterCount++;

	m_bgdPxls.clear(); m_fgdPxls.clear();
	m_prBgdPxls.clear(); m_prFgdPxls.clear();

	return m_iterCount;
}

BuildingDetector gcapp;
void on_mouse(int event, int x, int y, int flags, void* param)
{
	gcapp.mouseClick(event, x, y, flags, param);
}

int _tmain(int argc, _TCHAR* argv[])
{
	string filename = "../images/test.png";
	Mat img = imread(filename);
	//if (img.empty())
	//{
	//	std::cout << "\n Error, couldn't read m_image with filename " << filename << endl;
	//	return 1;
	//}

	const string m_winName = "image";
	cvNamedWindow(m_winName.c_str(), CV_WINDOW_AUTOSIZE);
	cvSetMouseCallback(m_winName.c_str(), on_mouse, 0);

	gcapp.setImageAndWinName(img, m_winName);
	gcapp.showImage();

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
			gcapp.reset();
			gcapp.showImage();
			break;
		case 'b':

			break;
		case 'n':
			int m_iterCount = gcapp.getIterCount();
			cout << "<" << m_iterCount << "... ";
			int newIterCount = gcapp.nextIter();
			if (newIterCount > m_iterCount)
			{
				gcapp.showImage();
				cout << m_iterCount << ">" << endl;
			}
			else
				cout << "m_rect must be determined>" << endl;
			break;
		}
	}

exit_main:
	cvDestroyWindow(m_winName.c_str());
	return 0;
}

