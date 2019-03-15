// dear imgui, v1.50 WIP
// (headers)

// These wrapper functions exist solely to circumvent a current bug
// somewhere in the interface between C++ and D

#include "imgui.h"
#include "imgui_wrapper.h"

// ImGui end-user API
// In a namespace so that user can add extra functions in a separate file (e.g. Value() helpers for your vector or common types)

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
