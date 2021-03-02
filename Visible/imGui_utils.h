//
//  imGui_utils.h
//  Visible
//
//  Created by Arman Garakani on 1/25/21.
//

#ifndef imGui_utils_h
#define imGui_utils_h

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

#include <vector>
#include <opencv2/opencv.hpp>
#include <imgui.h>

using namespace std;
using namespace cv;

#ifndef IMGUI_DEFINE_MATH_OPERATORS
static ImVec2 operator+(const ImVec2 &a, const ImVec2 &b) {
	return ImVec2(a.x + b.x, a.y + b.y);
}

static ImVec2 operator-(const ImVec2 &a, const ImVec2 &b) {
	return ImVec2(a.x - b.x, a.y - b.y);
}

	//static ImVec2 operator*(const ImVec2 &a, const ImVec2 &b) {
	//    return ImVec2(a.x * b.x, a.y * b.y);
	//}
	//
	//static ImVec2 operator/(const ImVec2 &a, const ImVec2 &b) {
	//    return ImVec2(a.x / b.x, a.y / b.y);
	//}

static ImVec2 operator*(const ImVec2 &a, const float b) {
	return ImVec2(a.x * b, a.y * b);
}
#endif


void rotatedRect2ImGui(const cv::RotatedRect& rr, std::vector<ImVec2> impts);
ImVec2 ImGui_Image_Constrain(const ImVec2&, const ImVec2& available);

template <typename T>
inline T RandomRange(T min, T max) {
	T scale = rand() / (T) RAND_MAX;
	return min + scale * ( max - min );
}

	// utility structure for realtime plot
struct ScrollingBuffer {
	int MaxSize;
	int Offset;
	ImVector<ImVec2> Data;
	ScrollingBuffer(int max_size = 2000) {
		MaxSize = max_size;
		Offset  = 0;
		Data.reserve(MaxSize);
	}
	void AddPoint(float x, float y) {
		if (Data.size() < MaxSize)
			Data.push_back(ImVec2(x,y));
		else {
			Data[Offset] = ImVec2(x,y);
			Offset =  (Offset + 1) % MaxSize;
		}
	}
	void Erase() {
		if (Data.size() > 0) {
			Data.shrink(0);
			Offset  = 0;
		}
	}
};

#pragma GCC diagnostic pop


#endif /* imGui_utils_h */
