
#ifndef BaseGUI_h
#define BaseGUI_h

#include "gui_handler.hpp"
#include "CinderImGui.h"
#include <stdio.h>

class gui_base {
public:
    /**
     This should be used to set up GUI variables in any class that interfaces with the GUI.
     */
    virtual void SetupGUIVariables() = 0;
    gui_handler& GetGUI(){
        return gui_handler::GetInstance();
    }
    
    virtual void DrawGUI() = 0;
    
    bool showGUI = false;
    
    static void ShowHelpMarker(const char* desc)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", desc);
    }
    
    
};


#endif /* BaseGUI_h */
