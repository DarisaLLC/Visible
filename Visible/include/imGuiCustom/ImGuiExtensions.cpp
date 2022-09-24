//
//  ImGuiExtensions.cpp
//  Visible
//
//  Created by Arman Garakani on 9/17/19.
//

#include <stdio.h>
#include "ImGuiExtensions.h"
//#include "imgui_internal.h"
using namespace ImGui;

bool ImGui::InputInt64 (const char* label, int64_t* v, int64_t step, int64_t step_fast, ImGuiInputTextFlags flags)
{
    // Hexadecimal input provided as a convenience but the flag name is awkward. Typically you'd use InputText() to parse your own data, if you want to handle prefixes.
    const char* format = (flags & ImGuiInputTextFlags_CharsHexadecimal) ? "%08X" : "%ll";
    return InputScalar(label, ImGuiDataType_S64, (void*)v, (void*)(step>0 ? &step : NULL), (void*)(step_fast>0 ? &step_fast : NULL), format, flags);
}
