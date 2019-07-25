// dear imgui, v1.50 WIP
// (headers)

// These wrapper functions exist solely to circumvent a current bug
// somewhere in the interface between C++ and D

#include "imgui.h"
#include "imgui_wrapper.h"

// ImGui end-user API
// In a namespace so that user can add extra functions in a separate file (e.g. Value() helpers for your vector or common types)


// generic miniDart theme
void ImGui::StyleColorsLightGreen(ImGuiStyle* dst)
{
    ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
    ImVec4* colors = style->Colors;
    
    style->WindowRounding    = 2.0f;             // Radius of window corners rounding. Set to 0.0f to have rectangular windows
    style->ScrollbarRounding = 3.0f;             // Radius of grab corners rounding for scrollbar
    style->GrabRounding      = 2.0f;             // Radius of grabs corners rounding. Set to 0.0f to have rectangular slider grabs.
    style->AntiAliasedLines  = true;
    style->AntiAliasedFill   = true;
    style->WindowRounding    = 2;
    style->ChildRounding     = 2;
    style->ScrollbarSize     = 16;
    style->ScrollbarRounding = 3;
    style->GrabRounding      = 2;
    style->ItemSpacing.x     = 10;
    style->ItemSpacing.y     = 4;
    style->IndentSpacing     = 22;
    style->FramePadding.x    = 6;
    style->FramePadding.y    = 4;
    style->Alpha             = 1.0f;
    style->FrameRounding     = 3.0f;
    
    colors[ImGuiCol_Text]                   = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    //colors[ImGuiCol_ChildWindowBg]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.93f, 0.93f, 0.93f, 0.98f);
    colors[ImGuiCol_Border]                = ImVec4(0.71f, 0.71f, 0.71f, 0.08f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.04f);
    colors[ImGuiCol_FrameBg]               = ImVec4(0.71f, 0.71f, 0.71f, 0.55f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.94f, 0.94f, 0.94f, 0.55f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.71f, 0.78f, 0.69f, 0.98f);
    colors[ImGuiCol_TitleBg]               = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.82f, 0.78f, 0.78f, 0.51f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.78f, 0.78f, 0.78f, 1.00f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.20f, 0.25f, 0.30f, 0.61f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.90f, 0.90f, 0.90f, 0.30f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.92f, 0.92f, 0.92f, 0.78f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_CheckMark]             = ImVec4(0.184f, 0.407f, 0.193f, 1.00f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Button]                = ImVec4(0.71f, 0.78f, 0.69f, 0.40f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.725f, 0.805f, 0.702f, 1.00f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.793f, 0.900f, 0.836f, 1.00f);
    colors[ImGuiCol_Header]                = ImVec4(0.71f, 0.78f, 0.69f, 0.31f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.71f, 0.78f, 0.69f, 0.80f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.71f, 0.78f, 0.69f, 1.00f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator]              = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.14f, 0.44f, 0.80f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.14f, 0.44f, 0.80f, 1.00f);
    colors[ImGuiCol_ResizeGrip]            = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.26f, 0.59f, 0.98f, 0.45f);
    colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
    colors[ImGuiCol_PlotLines]             = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_ModalWindowDarkening]  = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_NavHighlight]           = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(0.70f, 0.70f, 0.70f, 0.70f);
}

void ImGui::GetContentRegionMax(ImVec2& result) {                                              // current content boundaries (typically window boundaries including scrolling, or current column boundaries), in windows coordinates
    result = GetContentRegionMax();
}

void ImGui::GetContentRegionAvail(ImVec2& result) {                                            // == GetContentRegionMax() - GetCursorPos()
    result = GetContentRegionAvail();
}

void ImGui::GetWindowContentRegionMin(ImVec2& result) {                                        // content boundaries min (roughly (0,0)-Scroll), in window coordinates
    result = GetWindowContentRegionMin();
}

void ImGui::GetWindowContentRegionMax(ImVec2& result) {                                        // content boundaries max (roughly (0,0)+Size-Scroll) where Size can be override with SetNextWindowContentSize(), in window coordinates
    result = GetWindowContentRegionMax();
}

void ImGui::GetWindowPos(ImVec2& result) {                                                     // get current window position in screen space (useful if you want to do your own drawing via the DrawList api)
    result = GetWindowPos();
}

void ImGui::GetWindowSize(ImVec2& result) {                                                    // get current window size
    result = GetWindowSize();
}

void ImGui::GetFontTexUvWhitePixel(ImVec2& result) {                                           // get UV coordinate for a while pixel, useful to draw custom shapes via the ImDrawList API
    result = GetFontTexUvWhitePixel();
}

void ImGui::GetCursorPos(ImVec2& result) {                                                     // cursor position is relative to window position
    result = GetCursorPos();
}

void ImGui::GetCursorStartPos(ImVec2& result) {                                                // initial cursor position
    result = GetCursorStartPos();
}

void ImGui::GetCursorScreenPos(ImVec2& result) {                                               // cursor position in absolute screen coordinates [0..io.DisplaySize] (useful to work with ImDrawList API)
    result = GetCursorScreenPos();
}

void ImGui::GetItemRectMin(ImVec2& result) {                                                   // get bounding rect of last item in screen space
    result = GetItemRectMin();
}

void ImGui::GetItemRectMax(ImVec2& result) {                                                   // "
    result = GetItemRectMax();
}

void ImGui::GetItemRectSize(ImVec2& result) {                                                  // "
    result = GetItemRectSize();
}

void ImGui::CalcItemRectClosestPoint(ImVec2& result, const ImVec2& pos, bool on_edge, float outward) {   // utility to find the closest point the last item bounding rectangle edge. useful to visually link items
    result = CalcItemRectClosestPoint(pos, on_edge, outward);
}

void ImGui::CalcTextSize(ImVec2& result, const char* text, const char* text_end, bool hide_text_after_double_hash, float wrap_width) {
    result = CalcTextSize(text, text_end, hide_text_after_double_hash, wrap_width);
}

void ImGui::GetMousePos(ImVec2& result) {                                                      // shortcut to void ImGui::GetIO(ImVec2& result).MousePos provided by user, to be consistent with other calls
    result = GetMousePos();
}

void ImGui::GetMousePosOnOpeningCurrentPopup(ImVec2& result) {                                 // retrieve backup of mouse positioning at the time of opening popup we have BeginPopup() into
    result = GetMousePosOnOpeningCurrentPopup();
}

void ImGui::GetMouseDragDelta(ImVec2& result, int button, float lock_threshold) {    // dragging amount since clicking. if lock_threshold < -1.0f uses io.MouseDraggingThreshold
    result = GetMouseDragDelta(button, lock_threshold);
}
/*
 void ImGui::CalcTextSizeA(ImVec2& result, float size, float max_width, float wrap_width, const char* text_begin, const char* text_end = NULL, const char** remaining = NULL) const { // utf8
 result = CalcTextSizeA(size, max_width, wrap_width, text_begin, text_end, remaining);
 }
 */
