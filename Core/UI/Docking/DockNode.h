#pragma once

#include "../Framework/Panel.h"
#include <memory>
#include <vector>
#include <string>
#include <imgui.h>

namespace BGE {

enum class DockDirection {
    None,
    Left,
    Right,
    Top,
    Bottom,
    Center
};

enum class DockNodeType {
    Root,       // The main application docking area
    Split,      // Container with two children (horizontal or vertical split)
    Leaf,       // Container holding actual panels (tabs)
    Floating    // Floating window
};

struct DockNodeData {
    ImVec2 position{0, 0};
    ImVec2 size{0, 0};
    float splitRatio = 0.5f;  // For split nodes: ratio between children
    bool isHorizontalSplit = true;  // For split nodes: true = left/right, false = top/bottom
    int activeTabIndex = 0;   // For leaf nodes: which panel tab is active
};

class DockNode : public std::enable_shared_from_this<DockNode> {
public:
    DockNode(DockNodeType type = DockNodeType::Leaf);
    ~DockNode() = default;

    // Tree structure
    void SetParent(std::shared_ptr<DockNode> parent) { m_parent = parent; }
    std::shared_ptr<DockNode> GetParent() const { return m_parent.lock(); }
    
    void AddChild(std::shared_ptr<DockNode> child);
    void RemoveChild(std::shared_ptr<DockNode> child);
    const std::vector<std::shared_ptr<DockNode>>& GetChildren() const { return m_children; }
    
    // Panel management (for leaf nodes)
    void AddPanel(std::shared_ptr<Panel> panel);
    void RemovePanel(const std::string& panelName);
    void RemovePanel(std::shared_ptr<Panel> panel);
    bool HasPanel(const std::string& panelName) const;
    const std::vector<std::shared_ptr<Panel>>& GetPanels() const { return m_panels; }
    
    // Active tab management
    void SetActiveTab(int index);
    void SetActiveTab(const std::string& panelName);
    int GetActiveTabIndex() const { return m_data.activeTabIndex; }
    std::shared_ptr<Panel> GetActivePanel() const;
    
    // Node properties
    DockNodeType GetType() const { return m_type; }
    void SetType(DockNodeType type) { m_type = type; }
    
    const std::string& GetId() const { return m_id; }
    void SetId(const std::string& id) { m_id = id; }
    
    // Layout data
    DockNodeData& GetData() { return m_data; }
    const DockNodeData& GetData() const { return m_data; }
    
    // Utility methods
    bool IsEmpty() const;
    bool IsLeaf() const { return m_type == DockNodeType::Leaf; }
    bool IsSplit() const { return m_type == DockNodeType::Split; }
    bool IsRoot() const { return m_type == DockNodeType::Root; }
    bool IsFloating() const { return m_type == DockNodeType::Floating; }
    
    // Split creation
    std::shared_ptr<DockNode> Split(DockDirection direction, std::shared_ptr<Panel> newPanel);
    
    // Cleanup empty nodes
    void Cleanup();
    
    // Find node containing a specific panel
    std::shared_ptr<DockNode> FindNodeWithPanel(const std::string& panelName);
    std::shared_ptr<DockNode> FindNodeWithPanel(std::shared_ptr<Panel> panel);

private:
    DockNodeType m_type;
    std::string m_id;
    DockNodeData m_data;
    
    std::weak_ptr<DockNode> m_parent;
    std::vector<std::shared_ptr<DockNode>> m_children;
    std::vector<std::shared_ptr<Panel>> m_panels;  // Only used for leaf nodes
    
    static int s_nextId;
    void GenerateId();
};

} // namespace BGE