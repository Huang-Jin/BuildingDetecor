#include "stdafx.h"
#include <opencv2/opencv.hpp>

extern "C"
{
	_declspec(dllexport) cv::Mat building_detector_rect(std::string filename, int x, int y, int w, int h);
	typedef cv::Mat(*BuildDetRec)(std::string, int, int, int, int);
}

extern "C"
{
	_declspec(dllexport) cv::Mat building_detector_pt(std::string filename, int x, int y, int w, int h,
		std::vector<cv::Point> fgd_pts, std::vector<cv::Point> bgd_pts);
	typedef cv::Mat(*BuildDetPt)(std::string, int, int, int, int, std::vector<cv::Point>, std::vector<cv::Point>);
}

