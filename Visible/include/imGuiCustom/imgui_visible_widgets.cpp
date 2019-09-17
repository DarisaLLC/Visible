//
//  imgui_visible_widgets.cpp
//  Visible
//
//  Created by Arman Garakani on 9/1/19.
//

#include "imgui_visible_widgets.hpp"
#include <imgui.h>
#include "imgui_plot.h"
#include "core/core.hpp"

using namespace ImGui;


#include <imgui.h>
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui_internal.h>

namespace ImGui {
    
    int DrawCross(ImDrawList* draw_list, ImVec4 color, const ImVec2 size, bool editing, ImVec2 pos)
    {
        ImGuiIO& io = ImGui::GetIO();
        
        ImVec2 p1 = ImLerp(pos, ImVec2(pos + ImVec2(size.x - size.y, 0.f)), color.w) + ImVec2(3, 3);
        ImVec2 p2 = ImLerp(pos + ImVec2(size.y, size.y), ImVec2(pos + size), color.w) - ImVec2(3, 3);
        ImRect rc(p1, p2);
        ImVec2 mt = (rc.GetTL() + rc.GetTR()) / 2.0f;
        mt = mt - ImVec2(0, 3);
        ImVec2 mb = (rc.GetBL() + rc.GetBR()) / 2.0f;
        mb = mb + ImVec2(0, 3);
        ImVec2 ml = (rc.GetTL() + rc.GetBL()) / 2.0f;
        ml = ml - ImVec2(3, 0);
        ImVec2 mr = (rc.GetTR() + rc.GetBR()) / 2.0f;
        mr = mr + ImVec2(3, 0);
        
        color.w = 1.f;
        draw_list->AddRect(p1, p2, ImColor(color));
        draw_list->AddLine(mt, mb, ImColor(0,0,0));
        draw_list->AddLine(ml, mr, ImColor(0,0,0));
        
        if (editing)
            draw_list->AddRect(p1, p2, 0xFFFFFFFF, 2.f, 15, 2.5f);
            else
                draw_list->AddRect(p1, p2, 0x80FFFFFF, 2.f, 15, 1.25f);
                
                if (rc.Contains(io.MousePos))
                {
                    if (io.MouseClicked[0])
                        return 2;
                    return 1;
                }
        return 0;
    }

    // [0..1] -> [0..1]
    static float rescale(float t, float min, float max, ContractionPlotConfig::Scale::Type type) {
        switch (type) {
            case ContractionPlotConfig::Scale::Linear:
                return t;
            case ContractionPlotConfig::Scale::Log10:
                return log10(ImLerp(min, max, t) / min) / log10(max / min);
        }
        return 0;
    }
    
    // [0..1] -> [0..1]
    static float rescale_inv(float t, float min, float max, ContractionPlotConfig::Scale::Type type) {
        switch (type) {
            case ContractionPlotConfig::Scale::Linear:
                return t;
            case ContractionPlotConfig::Scale::Log10:
                return (pow(max/min, t) * min - min) / (max - min);
        }
        return 0;
    }
    
    static int cursor_to_idx(const ImVec2& pos, const ImRect& bb, const ContractionPlotConfig& conf, float x_min, float x_max) {
        const float t = ImClamp((pos.x - bb.Min.x) / (bb.Max.x - bb.Min.x), 0.0f, 0.9999f);
        const int v_idx = (int)(rescale_inv(t, x_min, x_max, conf.scale.type) * (conf.values.count - 1));
        IM_ASSERT(v_idx >= 0 && v_idx < conf.values.count);
        
        return v_idx;
    }
    
    Status Plot(const char* label, const ContractionPlotConfig& conf) {
        Status status = Status::nothing;
        
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return status;
        
        const float* const* ys_list = conf.values.ys_list;
        int ys_count = conf.values.ys_count;
        const ImU32* colors = conf.values.colors;
        if (conf.values.ys != nullptr) { // draw only a single plot
            ys_list = &conf.values.ys;
            ys_count = 1;
            colors = &conf.values.color;
        }
        
        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);
        
        const ImRect frame_bb(
                              window->DC.CursorPos,
                              window->DC.CursorPos + conf.frame_size);
        const ImRect inner_bb(
                              frame_bb.Min + style.FramePadding,
                              frame_bb.Max - style.FramePadding);
        const ImRect total_bb = frame_bb;
        ItemSize(total_bb, style.FramePadding.y);
        if (!ItemAdd(total_bb, 0, &frame_bb))
            return status;
        const bool hovered = ItemHoverable(frame_bb, id);
        
        RenderFrame(
                    frame_bb.Min,
                    frame_bb.Max,
                    GetColorU32(ImGuiCol_FrameBg),
                    true,
                    style.FrameRounding);
        
        if (conf.values.count > 0) {
            int res_w;
            if (conf.skip_small_lines)
                res_w = ImMin((int)conf.frame_size.x, conf.values.count);
            else
                res_w = conf.values.count;
            res_w -= 1;
            int item_count = conf.values.count - 1;
            
            float x_min = conf.values.offset;
            float x_max = conf.values.offset + conf.values.count - 1;
            if (conf.values.xs) {
                x_min = conf.values.xs[size_t(x_min)];
                x_max = conf.values.xs[size_t(x_max)];
            }
            
            // Tooltip on hover
            int v_hovered = -1;
            if (conf.tooltip.show && hovered && inner_bb.Contains(g.IO.MousePos)) {
                const int v_idx = cursor_to_idx(g.IO.MousePos, inner_bb, conf, x_min, x_max);
                const size_t data_idx = conf.values.offset + (v_idx % conf.values.count);
                const float x0 = conf.values.xs ? conf.values.xs[data_idx] : v_idx;
                const float y0 = ys_list[0][data_idx]; // TODO: tooltip is only shown for the first y-value!
                SetTooltip(conf.tooltip.format, x0, y0);
                v_hovered = v_idx;
            }
            
            const float t_step = 1.0f / (float)res_w;
            const float inv_scale = (conf.scale.min == conf.scale.max) ?
            0.0f : (1.0f / (conf.scale.max - conf.scale.min));
            
            if (conf.grid_x.show) {
                int y0 = inner_bb.Min.y;
                int y1 = inner_bb.Max.y;
                switch (conf.scale.type) {
                    case ContractionPlotConfig::Scale::Linear: {
                        float cnt = conf.values.count / (conf.grid_x.size / conf.grid_x.subticks);
                        float inc = 1.f / cnt;
                        for (int i = 0; i <= cnt; ++i) {
                            int x0 = ImLerp(inner_bb.Min.x, inner_bb.Max.x, i * inc);
                            window->DrawList->AddLine(
                                                      ImVec2(x0, y0),
                                                      ImVec2(x0, y1),
                                                      IM_COL32(200, 200, 200, (i % conf.grid_x.subticks) ? 128 : 255));
                        }
                        break;
                    }
                    case ContractionPlotConfig::Scale::Log10: {
                        float start = 1.f;
                        while (start < x_max) {
                            for (int i = 1; i < 10; ++i) {
                                float x = start * i;
                                if (x < x_min) continue;
                                if (x > x_max) break;
                                float t = log10(x / x_min) / log10(x_max / x_min);
                                int x0 = ImLerp(inner_bb.Min.x, inner_bb.Max.x, t);
                                window->DrawList->AddLine(
                                                          ImVec2(x0, y0),
                                                          ImVec2(x0, y1),
                                                          IM_COL32(200, 200, 200, (i > 1) ? 128 : 255));
                            }
                            start *= 10.f;
                        }
                        break;
                    }
                }
            }
            if (conf.grid_y.show) {
                int x0 = inner_bb.Min.x;
                int x1 = inner_bb.Max.x;
                float cnt = (conf.scale.max - conf.scale.min) / (conf.grid_y.size / conf.grid_y.subticks);
                float inc = 1.f / cnt;
                for (int i = 0; i <= cnt; ++i) {
                    int y0 = ImLerp(inner_bb.Min.y, inner_bb.Max.y, i * inc);
                    window->DrawList->AddLine(
                                              ImVec2(x0, y0),
                                              ImVec2(x1, y0),
                                              IM_COL32(0, 0, 0, (i % conf.grid_y.subticks) ? 16 : 64));
                }
            }
            
            const ImU32 col_hovered = GetColorU32(ImGuiCol_PlotLinesHovered);
            ImU32 col_base = GetColorU32(ImGuiCol_PlotLines);
            
            for (int i = 0; i < ys_count; ++i) {
                if (colors) {
                    if (colors[i]) col_base = colors[i];
                    else col_base = GetColorU32(ImGuiCol_PlotLines);
                }
                float v0 = ys_list[i][conf.values.offset];
                float t0 = 0.0f;
                // Point in the normalized space of our target rectangle
                ImVec2 tp0 = ImVec2(t0, 1.0f - ImSaturate((v0 - conf.scale.min) * inv_scale));
                
                for (int n = 0; n < res_w; n++)
                {
                    const float t1 = t0 + t_step;
                    const int v1_idx = (int)(t0 * item_count + 0.5f);
                    IM_ASSERT(v1_idx >= 0 && v1_idx < conf.values.count);
                    const float v1 = ys_list[i][conf.values.offset + (v1_idx + 1) % conf.values.count];
                    const ImVec2 tp1 = ImVec2(
                                              rescale(t1, x_min, x_max, conf.scale.type),
                                              1.0f - ImSaturate((v1 - conf.scale.min) * inv_scale));
                    
                    // NB: Draw calls are merged together by the DrawList system. Still, we should render our batch are lower level to save a bit of CPU.
                    ImVec2 pos0 = ImLerp(inner_bb.Min, inner_bb.Max, tp0);
                    ImVec2 pos1 = ImLerp(inner_bb.Min, inner_bb.Max, tp1);
                    
                    if (v1_idx == v_hovered) {
                        window->DrawList->AddCircleFilled(pos0, 3, col_hovered);
                    }
                    
                    window->DrawList->AddLine(
                                              pos0,
                                              pos1,
                                              col_base,
                                              conf.line_thickness);
                    
                    t0 = t1;
                    tp0 = tp1;
                }
            }
            
            if (conf.v_lines.show) {
                for (size_t i = 0; i < conf.v_lines.count; ++i) {
                    const size_t idx = conf.v_lines.indices[i];
                    const float t1 = rescale(idx * t_step, x_min, x_max, conf.scale.type);
                    ImVec2 pos0 = ImLerp(inner_bb.Min, inner_bb.Max, ImVec2(t1, 0.f));
                    ImVec2 pos1 = ImLerp(inner_bb.Min, inner_bb.Max, ImVec2(t1, 1.f));
                    window->DrawList->AddLine(pos0, pos1, IM_COL32(0xff, 0, 0, 0x88));
                }
            }
            
            if (conf.selection.show) {
                if (hovered) {
                    if (g.IO.MouseClicked[0]) {
                        SetActiveID(id, window);
                        FocusWindow(window);
                        
                        const int v_idx = cursor_to_idx(g.IO.MousePos, inner_bb, conf, x_min, x_max);
                        uint32_t start = conf.values.offset + (v_idx % conf.values.count);
                        uint32_t end = start;
                        if (conf.selection.sanitize_fn)
                            end = conf.selection.sanitize_fn(end - start) + start;
                        if (end < conf.values.offset + conf.values.count) {
                            *conf.selection.start = start;
                            *conf.selection.length = end - start;
                            status = Status::selection_updated;
                        }
                    }
                }
                
                if (g.ActiveId == id) {
                    if (g.IO.MouseDown[0]) {
                        const int v_idx = cursor_to_idx(g.IO.MousePos, inner_bb, conf, x_min, x_max);
                        const uint32_t start = *conf.selection.start;
                        uint32_t end = conf.values.offset + (v_idx % conf.values.count);
                        if (end > start) {
                            if (conf.selection.sanitize_fn)
                                end = conf.selection.sanitize_fn(end - start) + start;
                            if (end < conf.values.offset + conf.values.count) {
                                *conf.selection.length = end - start;
                                status = Status::selection_updated;
                            }
                        }
                    } else {
                        ClearActiveID();
                    }
                }
                
                ImVec2 pos0 = ImLerp(inner_bb.Min, inner_bb.Max,
                                     ImVec2(t_step * *conf.selection.start, 0.f));
                ImVec2 pos1 = ImLerp(inner_bb.Min, inner_bb.Max,
                                     ImVec2(t_step * (*conf.selection.start + *conf.selection.length), 1.f));
                window->DrawList->AddRectFilled(pos0, pos1, IM_COL32(128, 128, 128, 32));
                window->DrawList->AddRect(pos0, pos1, IM_COL32(128, 128, 128, 128));
            }
        }
        
        // Text overlay
        if (conf.overlay_text)
            RenderTextClipped(ImVec2(frame_bb.Min.x, frame_bb.Min.y + style.FramePadding.y), frame_bb.Max, conf.overlay_text, NULL, NULL, ImVec2(0.5f,0.0f));
        
        return status;
    }
}

//
//static void HelpMarker(const char* desc)
//{
//    ImGui::TextDisabled("(?)");
//    if (ImGui::IsItemHovered())
//    {
//        ImGui::BeginTooltip();
//        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
//        ImGui::TextUnformatted(desc);
//        ImGui::PopTextWrapPos();
//        ImGui::EndTooltip();
//    }
//}
//

#if 0
void generate_data() {
    constexpr float sampling_freq = 44100;
    constexpr float freq = 500;
    x_data.resize(buf_size);
    y_data1.resize(buf_size);
    y_data2.resize(buf_size);
    y_data3.resize(buf_size);
    for (size_t i = 0; i < buf_size; ++i) {
        const float t = i / sampling_freq;
        x_data[i] = t;
        const float arg = 2 * M_PI * freq * t;
        y_data1[i] = sin(arg);
        y_data2[i] = y_data1[i] * -0.6 + sin(2 * arg) * 0.4;
        y_data3[i] = y_data2[i] * -0.6 + sin(3 * arg) * 0.4;
    }
}




void draw_multi_plot() {
    const float* y_data[] = { y_data1.data(), y_data2.data(), y_data3.data() };
    static ImU32 colors[3] = { ImColor(0, 255, 0), ImColor(255, 0, 0), ImColor(0, 0, 255) };
    static uint32_t selection_start = 0, selection_length = 0;
    
    ImGui::Begin("Example plot", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    // Draw first plot with multiple sources
    ImGui::PlotConfig conf;
    conf.values.xs = x_data.data();
    conf.values.count = buf_size;
    conf.values.ys_list = y_data; // use ys_list to draw several lines simultaneously
    conf.values.ys_count = 3;
    conf.values.colors = colors;
    conf.scale.min = -1;
    conf.scale.max = 1;
    conf.tooltip.show = true;
    conf.grid_x.show = true;
    conf.grid_x.size = 128;
    conf.grid_x.subticks = 4;
    conf.grid_y.show = true;
    conf.grid_y.size = 0.5f;
    conf.grid_y.subticks = 5;
    conf.selection.show = true;
    conf.selection.start = &selection_start;
    conf.selection.length = &selection_length;
    conf.frame_size = ImVec2(buf_size, 200);
    ImGui::Plot("plot1", conf);
    
    // Draw second plot with the selection
    // reset previous values
    conf.values.ys_list = nullptr;
    conf.selection.show = false;
    // set new ones
    conf.values.ys = y_data3.data();
    conf.values.offset = selection_start;
    conf.values.count = selection_length;
    conf.line_thickness = 2.f;
    ImGui::Plot("plot2", conf);
    
    ImGui::End();
}

#endif

