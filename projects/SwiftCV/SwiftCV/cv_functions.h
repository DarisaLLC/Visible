
#ifndef CVOpenTemplate_Header_h
#define CVOpenTemplate_Header_h
#include <opencv2/opencv.hpp>

cv::Mat stitch (std::vector <cv::Mat> & images);

void compute_image_overlay (const cv::Mat& src, cv::Mat& mask);

#endif
