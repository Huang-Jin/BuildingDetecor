// BuildingDetecor.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include <opencv2/opencv.hpp>
#include "BuildingDetector.h"

using namespace cv;

Mat setRectInMask(Mat image, Rect rect, bool addCenter);
void setPtsInMask(Mat& mask, vector<Point> pts, bool isPr);
void getBinaryMask(const Mat& comMask, Mat& binMask);
void writeMask(string filename);

void getBinaryMask(const Mat& comMask, Mat& binMask)
{
	if (comMask.empty() || comMask.type() != CV_8UC1)
		CV_Error(CV_StsBadArg, "comMask is empty or has incorrect type (not CV_8UC1)");
	if (binMask.empty() || binMask.rows != comMask.rows || binMask.cols != comMask.cols)
		binMask.create(comMask.size(), CV_8UC1);
	binMask = comMask & 1;  //得到m_mask的最低位,实际上是只保留确定的或者有可能的前景点当做m_mask
}

void writeMask(string filename, Mat binMask)
{
	imwrite(filename, binMask * 255);
}

Mat getErrorMat()
{
	Mat mask;
	mask.create(Size(1, 1), CV_8UC1);
	mask.setTo(Scalar(0));
	return mask;
}

Mat setRectInMask(Mat image, Rect rect, bool addCenter)
{
	Mat mask;

	if (image.empty())
	{
		return mask;
	}

	mask.create(image.size(), CV_8UC1);
	mask.setTo(GC_BGD);   //GC_BGD == 0

	rect.x = max(0, rect.x);
	rect.y = max(0, rect.y);
	rect.width = min(rect.width, image.cols - rect.x);
	rect.height = min(rect.height, image.rows - rect.y);
	(mask(rect)).setTo(Scalar(GC_PR_FGD));    //GC_PR_FGD == 3，矩形内部,为可能的前景点

	if (addCenter)
	{
		mask.at<uchar>(rect.y + rect.height / 2, rect.x + rect.width / 2) = GC_FGD;
		//printf("position: %d %d : %d", rect.y + rect.height / 2, rect.x + rect.width / 2,
		//	mask.at<uchar>(rect.y + rect.height / 2, rect.x + rect.width / 2));
	}

	return mask;
}

void setPtsInMask(Mat& mask, vector<Point> pts, bool isFgd)
{
	int radius = 2;
	int thickness = -1;

	int value;
	if (isFgd)
	{
		value = GC_FGD;
	}
	else
	{
		value = GC_BGD;
	}

	for (int i = 0; i < pts.size(); i++)
	{
		circle(mask, pts[i], radius, value, thickness);
	}
}

void extractMask(Rect m_rect, int extent, Mat& image, Mat mask, Mat& ret_mask)
{
	Rect rect;
	rect.x = max(0, m_rect.x - extent);
	rect.y = max(0, m_rect.y - extent);
	rect.width = min(m_rect.width + extent * 2, image.cols - rect.x);
	rect.height = min(m_rect.height + extent * 2, image.rows - rect.y);

	Rect new_rect;
	new_rect.x = m_rect.x - rect.x;
	new_rect.y = m_rect.y - rect.y;
	new_rect.width = m_rect.width;
	new_rect.height = m_rect.height;

	Mat img_temp = image(rect);
	Mat mask_temp = mask(rect);
	Mat bgdModel, fgdModel;

	grabCut(img_temp, mask_temp, new_rect, bgdModel, fgdModel, 1, GC_INIT_WITH_MASK);
	mask(rect) = mask_temp;

	getBinaryMask(mask, ret_mask);
}

Mat building_detector_pt(string filename, int x, int y, int width, int height, vector<Point> fgd_pts, vector<Point> bgd_pts)
{
	Mat ret_mask;
	int extent = 10;

	if (filename.empty())
	{
		return ret_mask;
	}

	Mat image = imread(filename);
	Rect m_rect = Rect(x, y, width, height);
	if (m_rect.width <= 0 || m_rect.height <= 0)
	{
		return ret_mask;
	}

	Mat mask = setRectInMask(image, m_rect, true);
	setPtsInMask(mask, fgd_pts, true);
	setPtsInMask(mask, bgd_pts, false);

	extractMask(m_rect, extent, image, mask, ret_mask);
	return ret_mask;
}

Mat building_detector_rect(string filename, int x, int y, int width, int height)
{
	Mat ret_mask;
	int extent = 10;

	if (filename.empty())
	{
		return ret_mask;
	}

	Mat image = imread(filename);
	Rect m_rect = Rect(x, y, width, height);
	if (m_rect.width <= 0 || m_rect.height <= 0)
	{
		return ret_mask;
	}

	Mat mask = setRectInMask(image, m_rect, true);

	extractMask(m_rect, extent, image, mask, ret_mask);
	return ret_mask;
}