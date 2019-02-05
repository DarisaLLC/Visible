// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.


#ifndef IMGUIVARIOUSCONTROLS_H_
#define IMGUIVARIOUSCONTROLS_H_

#ifndef IMGUI_API
#include <imgui.h>
#endif //IMGUI_API

// USAGE
/*
#include "imguivariouscontrols.h"

// inside a ImGui::Window:
ImGui::TestProgressBar();

ImGui::TestPopupMenuSimple();
*/



namespace ImGui {
   // static void ImDrawListAddImageCircleFilled(ImDrawList* dl,ImTextureID user_texture_id,const ImVec2& uv0, const ImVec2& uv1,const ImVec2& centre, float radius, ImU32 col, int num_segments=8);

    bool ImageZoomAndPan(ImTextureID user_texture_id, const ImVec2& size,float aspectRatio,ImTextureID checkersTexID = NULL,float* pzoom=NULL,ImVec2* pzoomCenter=NULL,int panMouseButtonDrag=1,int resetZoomAndPanMouseButton=2,const ImVec2& zoomMaxAndZoomStep=ImVec2(16.f,1.025f));
    
// Returns the hovered value index WITH 'values_offset' ( (hovered_index+values_offset)%values_offset or -1). The index of the hovered histogram can be retrieved through 'pOptionalHoveredHistogramIndexOut'.
IMGUI_API int PlotHistogram(const char* label, const float** values,int num_histograms,int values_count, int values_offset = 0, const char* overlay_text = NULL, float scale_min = FLT_MAX, float scale_max = FLT_MAX, ImVec2 graph_size = ImVec2(0,0), int stride = sizeof(float),float histogramGroupSpacingInPixels=0.f,int* pOptionalHoveredHistogramIndexOut=NULL,float fillColorGradientDeltaIn0_05=0.05f,const ImU32* pColorsOverride=NULL,int numColorsOverride=0);
IMGUI_API int PlotHistogram(const char* label, float (*values_getter)(void* data, int idx,int histogramIdx), void* data,int num_histograms, int values_count, int values_offset = 0, const char* overlay_text = NULL, float scale_min = FLT_MAX, float scale_max = FLT_MAX, ImVec2 graph_size = ImVec2(0,0),float histogramGroupSpacingInPixels=0.f,int* pOptionalHoveredHistogramIndexOut=NULL,float fillColorGradientDeltaIn0_05=0.05f,const ImU32* pColorsOverride=NULL,int numColorsOverride=0);
// Shortcut for a single histogram to ease user code a bit (same signature as one of the 2 default Dear ImGui PlotHistogram(...) methods):
IMGUI_API int PlotHistogram2(const char* label, const float* values,int values_count, int values_offset = 0, const char* overlay_text = NULL, float scale_min = FLT_MAX, float scale_max = FLT_MAX, ImVec2 graph_size = ImVec2(0,0), int stride = sizeof(float),float fillColorGradientDeltaIn0_05=0.05f,const ImU32* pColorsOverride=NULL,int numColorsOverride=0);

// This one plots a generic function (or multiple functions together) of a float single variable.
// Returns the index of the hovered curve (or -1).
// Passing rangeX.y = FLT_MAX should ensure that the aspect ratio between axis is correct.
// By doubling 'precisionInPixels', we halve the times 'values_getter' gets called.
// 'numGridLinesHint' is currently something we must still fix. Set it to zero to hide lines.
IMGUI_API int PlotCurve(const char* label, float (*values_getter)(void* data, float x,int numCurve), void* data,int num_curves,const char* overlay_text,const ImVec2 rangeY,const ImVec2 rangeX=ImVec2(-.1f,FLT_MAX), ImVec2 graph_size=ImVec2(0,0),ImVec2* pOptionalHoveredValueOut=NULL,float precisionInPixels=1.f,float numGridLinesHint=4.f,const ImU32* pColorsOverride=NULL,int numColorsOverride=0);

// These 2 have a completely different implementation:
// Posted by @JaapSuter and @maxint (please see: https://github.com/ocornut/imgui/issues/632)
IMGUI_API void PlotMultiLines(
    const char* label,
    int num_datas,
    const char** names,
    const ImColor* colors,
    float(*getter)(const void* data, int idx),
    const void * const * datas,
    int values_count,
    float scale_min,
    float scale_max,
    ImVec2 graph_size);

// Posted by @JaapSuter and @maxint (please see: https://github.com/ocornut/imgui/issues/632)
IMGUI_API void PlotMultiHistograms(
    const char* label,
    int num_hists,
    const char** names,
    const ImColor* colors,
    float(*getter)(const void* data, int idx),
    const void * const * datas,
    int values_count,
    float scale_min,
    float scale_max,
    ImVec2 graph_size);

}

#endif
