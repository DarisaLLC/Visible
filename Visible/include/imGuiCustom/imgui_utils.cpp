//
//  imgui_utils.cpp
//  Visible
//
//  Created by Arman Garakani on 1/26/21.
//

#include "imGui_utils.h"


using namespace cv;
using namespace std;

void rotatedRect2ImGui(const cv::RotatedRect& rr, std::vector<ImVec2> impts){
    cv::Point2f corners[4];
    rr.points(corners);
    sort(corners, &corners[4],
         [](cv::Point2f p1, cv::Point2f p2) { return p1.y < p2.y; });
    impts.resize(0);
    for (int i = 0; i < 4; i++)
        impts.emplace_back(ImVec2(corners[i].x, corners[i].y));
}
