//
//  nms.hpp
//  Visible
//
//  Created by Arman Garakani on 2/2/21.
//

#ifndef nms_hpp
#define nms_hpp

#include <stdio.h>
#include <vector>
#include "opencv2/opencv.hpp"

using namespace std;
using namespace cv;

void nms(const std::vector<cv::Rect>& srcRects,
         std::vector<cv::Rect>& resRects,
         float thresh,
         int neighbors = 0);

void nms2(const std::vector<cv::Rect>& srcRects,
          const std::vector<float>& scores,
          std::vector<cv::Rect>& resRects,
          float thresh,
          int neighbors = 0,
          float minScoresSum = 0.f);

#endif /* nms_hpp */
