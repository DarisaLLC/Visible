//
//  imgui_common.h
//  Visible
//
//  Created by Arman Garakani on 9/16/19.
//

#ifndef imgui_common_h
#define imgui_common_h

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

#include "imgui.h"

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

#pragma GCC diagnostic pop

#endif /* imgui_common_h */
