// dear imgui, v1.50 WIP
// (headers)

// These wrapper functions exist solely to circumvent a current bug
// somewhere in the interface between C++ and D

#include "imgui/imgui.h"

// ImGui end-user API
// In a namespace so that user can add extra functions in a separate file (e.g. Value() helpers for your vector or common types)
namespace ImGui
{
    
     IMGUI_API void        StyleColorsLightGreen(ImGuiStyle* dst);
    
    IMGUI_API void        GetContentRegionMax(ImVec2& result);                                              // current content boundaries (typically window boundaries including scrolling, or current column boundaries), in windows coordinates
    IMGUI_API void        GetContentRegionAvail(ImVec2& result);                                            // == GetContentRegionMax() - GetCursorPos()
    IMGUI_API void        GetWindowContentRegionMin(ImVec2& result);                                        // content boundaries min (roughly (0,0)-Scroll), in window coordinates
    IMGUI_API void        GetWindowContentRegionMax(ImVec2& result);                                        // content boundaries max (roughly (0,0)+Size-Scroll) where Size can be override with SetNextWindowContentSize(), in window coordinates
    IMGUI_API void        GetWindowPos(ImVec2& result);                                                     // get current window position in screen space (useful if you want to do your own drawing via the DrawList api)
    IMGUI_API void        GetWindowSize(ImVec2& result);                                                    // get current window size
    IMGUI_API void        GetFontTexUvWhitePixel(ImVec2& result);                                           // get UV coordinate for a while pixel, useful to draw custom shapes via the ImDrawList API
    IMGUI_API void        GetCursorPos(ImVec2& result);                                                     // cursor position is relative to window position
    IMGUI_API void        GetCursorStartPos(ImVec2& result);                                                // initial cursor position
    IMGUI_API void        GetCursorScreenPos(ImVec2& result);                                               // cursor position in absolute screen coordinates [0..io.DisplaySize] (useful to work with ImDrawList API)
    IMGUI_API void        GetItemRectMin(ImVec2& result);                                                   // get bounding rect of last item in screen space
    IMGUI_API void        GetItemRectMax(ImVec2& result);                                                   // "
    IMGUI_API void        GetItemRectSize(ImVec2& result);                                                  // "
    IMGUI_API void        CalcItemRectClosestPoint(ImVec2& result, const ImVec2& pos, bool on_edge = false, float outward = +0.0f);   // utility to find the closest point the last item bounding rectangle edge. useful to visually link items
    IMGUI_API void        CalcTextSize(ImVec2& result, const char* text, const char* text_end = NULL, bool hide_text_after_double_hash = false, float wrap_width = -1.0f);
    IMGUI_API void        GetMousePos(ImVec2& result);                                                      // shortcut to ImGui::GetIO().MousePos provided by user, to be consistent with other calls
    IMGUI_API void        GetMousePosOnOpeningCurrentPopup(ImVec2& result);                                 // retrieve backup of mouse positioning at the time of opening popup we have BeginPopup() into
    IMGUI_API void        GetMouseDragDelta(ImVec2& result, int button = 0, float lock_threshold = -1.0f);    // dragging amount since clicking. if lock_threshold < -1.0f uses io.MouseDraggingThreshold
                                                                                                              //  IMGUI_API void        CalcTextSizeA(ImVec2& result, float size, float max_width, float wrap_width, const char* text_begin, const char* text_end = NULL, const char** remaining = NULL) const; // utf8
}
