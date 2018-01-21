#include "stdafx.h"
#include <opencv2/opencv.hpp>

extern "C"
{
	_declspec(dllexport) cv::Mat building_detector(std::string filename, int x, int y, int w, int h);
	typedef cv::Mat(*BuildDet)(std::string, int, int, int, int);
}