#pragma once
#include <stdint.h>
#include "imgui.h"
#include "imgui_internal.h"
#include <vector>


using namespace std;


namespace ImGui
{
        IMGUI_API bool          InputInt64(const char* label, int64_t* v, int64_t step = 1, int64_t step_fast = 7, ImGuiInputTextFlags flags = 0);
	
}

