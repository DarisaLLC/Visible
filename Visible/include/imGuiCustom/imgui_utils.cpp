//
//  imgui_utils.cpp
//  Visible
//
//  Created by Arman Garakani on 1/26/21.
//

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

#include "imGui_utils.h"
#include <vision/opencv_utils.hpp>

using namespace cv;
using namespace std;

void HelpMarker(const char* desc)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
		{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(450.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
		}
}


void rotatedRect2ImGui(const cv::RotatedRect& rr, std::vector<ImVec2> impts){
    std::vector<cv::Point2f> corners;
    compute2DRectCorners(rr, corners);
    impts.resize(0);
    for (int i = 0; i < 4; i++)
        impts.emplace_back(ImVec2(corners[i].x, corners[i].y));
}

ImVec2 ImGui_Image_Constrain(const ImVec2& content, const ImVec2& available)
{
	float availableRatio = available.x / available.y;
	float imageRatio = (float)content.x / (float)content.y;
	if(availableRatio > imageRatio) {
		// available space is wider, so we have extra space on the sides
		return ImVec2(imageRatio * available.y, available.y);
	} else if(availableRatio < imageRatio) {
		// available space is taller, so we have extra space on the top
		return ImVec2(available.x, available.x / imageRatio);
	} else {
		return available;
	}
}

#pragma GCC diagnostic pop
