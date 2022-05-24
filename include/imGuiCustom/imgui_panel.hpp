//
//  imgui_panel.hpp
//  Visible
//
//  Created by Arman Garakani on 2/24/21.
//

#ifndef imgui_panel_hpp
#define imgui_panel_hpp

// Thanks @thedmd
// From https://github.com/ocornut/imgui/issues/1496#issuecomment-655048353

#include "imgui.h"

namespace ImGui {

void BeginGroupPanel(const char* name, const ImVec2& size = ImVec2(0.0f, 0.0f));
void EndGroupPanel();

}  // namespace ImGui


#endif /* imgui_panel_hpp */
