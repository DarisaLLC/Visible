//
//  imGui_utils.h
//  Visible
//
//  Created by Arman Garakani on 1/25/21.
//

#ifndef imGui_utils_h
#define imGui_utils_h

#include <vector>
#include <opencv2/opencv.hpp>
#include <imgui.h>

using namespace std;
using namespace cv;

void rotatedRect2ImGui(const cv::RotatedRect& rr, std::vector<ImVec2> impts);


#endif /* imGui_utils_h */
