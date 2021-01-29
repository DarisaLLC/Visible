//
//  imgui_utils.cpp
//  Visible
//
//  Created by Arman Garakani on 1/26/21.
//

#include "imGui_utils.h"
#include <vision/opencv_utils.hpp>

using namespace cv;
using namespace std;

void rotatedRect2ImGui(const cv::RotatedRect& rr, std::vector<ImVec2> impts){
    std::vector<cv::Point2f> corners;
    compute2DRectCorners(rr, corners);
    impts.resize(0);
    for (int i = 0; i < 4; i++)
        impts.emplace_back(ImVec2(corners[i].x, corners[i].y));
}
