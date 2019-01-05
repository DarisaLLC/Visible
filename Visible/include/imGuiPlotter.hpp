//
//  imGuiPlotter.hpp
//  Visible
//
//  Created by Arman Garakani on 1/2/19.
//

#ifndef imGuiPlotter_hpp
#define imGuiPlotter_hpp

#include <stdio.h>
#include "CinderImGui.h"


//#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"

namespace ImGui {
    
    // Start PlotMultiLines(...) and PlotMultiHistograms(...)------------------------
    // by @JaapSuter and @maxint (please see: https://github.com/ocornut/imgui/issues/632)
    // I assume the work is public domain.
    
    // Look for the first entry larger in t than the current position
    enum struct BinFunction {
        CEIL,
        FLOOR,
        MIN,
        MAX,
        AVG,
        COUNT
    };
    
    enum struct ValueScale {
        LINEAR,
        LOG
    };
    
    void PlotMultiLines(
                        const char* label,
                        int num_datas,
                        const char** names,
                        const ImColor* colors,
                        float (*getterY)(const void* data, int idx),
                        size_t (*getterX)(const void* data, int idx),
                        const void* const* datas,
                        int values_count,
                        float scale_min,
                        float scale_max,
                        size_t domain_min,
                        size_t domain_max,
                        ImVec2 graph_size);
    
    void PlotMultiHistograms(
                             const char* label,
                             int num_hists,
                             const char** names,
                             const ImColor* colors,
                             float (*getterY)(const void* data, int idx),
                             size_t (*getterX)(const void* data, int idx),
                             const void* const* datas,
                             int values_count,
                             float scale_min,
                             float scale_max,
                             size_t domain_min,
                             size_t domain_max,
                             ImVec2 graph_size);
   

struct MultiplotData_F {
        int mod;
        size_t first;
        size_t size;
        const void* Y;
        const void* X;
    };
 
    template <typename T>
    struct HistoData {
        int mod;
        size_t first;
        size_t size;
        const T* points;
    };

} // namespace ImGui
#endif /* imGuiPlotter_hpp */
