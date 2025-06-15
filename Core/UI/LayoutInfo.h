#pragma once

#include <imgui.h>

namespace BGE {

struct LayoutInfo {
    ImVec4 leftArea;
    ImVec4 centerArea;
    ImVec4 rightArea;
    ImVec4 topToolbarArea;
    ImVec4 bottomArea;
};

} // namespace BGE