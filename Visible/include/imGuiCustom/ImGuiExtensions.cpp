
#include <stdint.h>
#include "ImGuiExtensions.h"



using namespace std;
using namespace ImGui;


VerticalGradient::VerticalGradient(const ImVec2& start, const ImVec2& end, const ImVec4& col0, const ImVec4& col1)
: Col0(col0)
, Col1(col1)
, Start(start)
, End(end)
{
    evalStep();
}

VerticalGradient::VerticalGradient(const ImVec2& start, const ImVec2& end, ImU32 col0, ImU32 col1)
: Col0(ColorConvertU32ToFloat4(col0))
, Col1(ColorConvertU32ToFloat4(col1))
, Start(start)
, End(end)
{
    evalStep();
}

void VerticalGradient::evalStep()
{
    Len = ImLengthSqr(End - Start);
}

ImU32 VerticalGradient::Calc(const ImVec2& pos) const
{
    const float fa = std::min(1.0f, (pos.y - Start.y)/(End.y - Start.y));
    const float fc = std::max(0.0f, (1.f - fa));
    
    return ColorConvertFloat4ToU32(ImVec4(
                                          Col0.x * fc + Col1.x * fa,
                                          Col0.y * fc + Col1.y * fa,
                                          Col0.z * fc + Col1.z * fa,
                                          Col0.w * fc + Col1.w * fa)
                                   );
}

bool ImGui::InputInt64 (const char* label, int64_t* v, int64_t step, int64_t step_fast, ImGuiInputTextFlags flags)
{
    // Hexadecimal input provided as a convenience but the flag name is awkward. Typically you'd use InputText() to parse your own data, if you want to handle prefixes.
    const char* format = (flags & ImGuiInputTextFlags_CharsHexadecimal) ? "%08X" : "%ll";
    return InputScalar(label, ImGuiDataType_S64, (void*)v, (void*)(step>0 ? &step : NULL), (void*)(step_fast>0 ? &step_fast : NULL), format, flags);
}

