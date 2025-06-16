#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <imgui.h>

namespace BGE {

enum class LayoutNodeType {
    Root = 0,
    Split = 1,
    Leaf = 2,
    Floating = 3
};

struct LayoutNodeData {
    LayoutNodeType type = LayoutNodeType::Leaf;
    ImVec2 position{0, 0};
    ImVec2 size{0, 0};
    float splitRatio = 0.5f;
    bool isHorizontalSplit = true;
    int activeTabIndex = 0;
    std::vector<std::string> panelNames;
    std::string nodeId;
    
    // For serialization
    std::vector<LayoutNodeData> children;
};

struct PersistentLayoutInfo {
    std::string name;
    std::string baseLayout = "Unity"; // Kept for backward compatibility
    LayoutNodeData rootNode;
    std::vector<LayoutNodeData> floatingNodes;
    std::unordered_map<std::string, bool> panelVisibility; // Track which panels should be visible
    
    // Serialize to JSON-like structure
    std::string Serialize() const;
    bool Deserialize(const std::string& data);
};

} // namespace BGE