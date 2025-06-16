#include "DockNode.h"
#include <algorithm>
#include <sstream>

namespace BGE {

int DockNode::s_nextId = 1;

DockNode::DockNode(DockNodeType type) 
    : m_type(type) {
    GenerateId();
}

void DockNode::GenerateId() {
    std::stringstream ss;
    ss << "DockNode_" << s_nextId++;
    m_id = ss.str();
}

void DockNode::AddChild(std::shared_ptr<DockNode> child) {
    if (child && std::find(m_children.begin(), m_children.end(), child) == m_children.end()) {
        m_children.push_back(child);
        child->SetParent(shared_from_this());
    }
}

void DockNode::RemoveChild(std::shared_ptr<DockNode> child) {
    auto it = std::find(m_children.begin(), m_children.end(), child);
    if (it != m_children.end()) {
        (*it)->SetParent(nullptr);
        m_children.erase(it);
    }
}

void DockNode::AddPanel(std::shared_ptr<Panel> panel) {
    if (!IsLeaf() || !panel) return;
    
    // Check if panel already exists
    auto it = std::find_if(m_panels.begin(), m_panels.end(),
        [&panel](const std::shared_ptr<Panel>& p) {
            return p && p->GetName() == panel->GetName();
        });
    
    if (it == m_panels.end()) {
        m_panels.push_back(panel);
        m_data.activeTabIndex = static_cast<int>(m_panels.size()) - 1;
    } else {
        // Panel already exists, just activate it
        m_data.activeTabIndex = static_cast<int>(std::distance(m_panels.begin(), it));
    }
}

void DockNode::RemovePanel(const std::string& panelName) {
    if (!IsLeaf()) return;
    
    auto it = std::find_if(m_panels.begin(), m_panels.end(),
        [&panelName](const std::shared_ptr<Panel>& panel) {
            return panel && panel->GetName() == panelName;
        });
    
    if (it != m_panels.end()) {
        int removedIndex = static_cast<int>(std::distance(m_panels.begin(), it));
        m_panels.erase(it);
        
        // Adjust active tab index
        if (m_data.activeTabIndex >= removedIndex && m_data.activeTabIndex > 0) {
            m_data.activeTabIndex--;
        }
        if (m_data.activeTabIndex >= static_cast<int>(m_panels.size())) {
            m_data.activeTabIndex = std::max(0, static_cast<int>(m_panels.size()) - 1);
        }
    }
}

void DockNode::RemovePanel(std::shared_ptr<Panel> panel) {
    if (panel) {
        RemovePanel(panel->GetName());
    }
}

bool DockNode::HasPanel(const std::string& panelName) const {
    if (!IsLeaf()) return false;
    
    return std::find_if(m_panels.begin(), m_panels.end(),
        [&panelName](const std::shared_ptr<Panel>& panel) {
            return panel && panel->GetName() == panelName;
        }) != m_panels.end();
}

void DockNode::SetActiveTab(int index) {
    if (IsLeaf() && index >= 0 && index < static_cast<int>(m_panels.size())) {
        m_data.activeTabIndex = index;
    }
}

void DockNode::SetActiveTab(const std::string& panelName) {
    if (!IsLeaf()) return;
    
    for (int i = 0; i < static_cast<int>(m_panels.size()); ++i) {
        if (m_panels[i] && m_panels[i]->GetName() == panelName) {
            m_data.activeTabIndex = i;
            break;
        }
    }
}

std::shared_ptr<Panel> DockNode::GetActivePanel() const {
    if (!IsLeaf() || m_data.activeTabIndex < 0 || 
        m_data.activeTabIndex >= static_cast<int>(m_panels.size())) {
        return nullptr;
    }
    return m_panels[m_data.activeTabIndex];
}

bool DockNode::IsEmpty() const {
    if (IsLeaf()) {
        return m_panels.empty();
    } else if (IsSplit()) {
        return m_children.empty() || 
               std::all_of(m_children.begin(), m_children.end(),
                   [](const std::shared_ptr<DockNode>& child) {
                       return !child || child->IsEmpty();
                   });
    }
    return true;
}

std::shared_ptr<DockNode> DockNode::Split(DockDirection direction, std::shared_ptr<Panel> newPanel) {
    if (!IsLeaf() || !newPanel) return nullptr;
    
    // Create new leaf node for the new panel
    auto newNode = std::make_shared<DockNode>(DockNodeType::Leaf);
    newNode->AddPanel(newPanel);
    
    // If this node is empty, just add the panel here instead of splitting
    if (IsEmpty()) {
        AddPanel(newPanel);
        return shared_from_this();
    }
    
    // Convert this node to a split node
    auto oldPanels = m_panels;
    auto oldActiveTab = m_data.activeTabIndex;
    
    // Create a new leaf node for existing panels
    auto existingNode = std::make_shared<DockNode>(DockNodeType::Leaf);
    existingNode->m_panels = oldPanels;
    existingNode->m_data.activeTabIndex = oldActiveTab;
    
    // Clear this node's panels and convert to split
    m_panels.clear();
    m_type = DockNodeType::Split;
    m_data.activeTabIndex = 0;
    
    // Set split orientation and add children
    bool isHorizontal = (direction == DockDirection::Left || direction == DockDirection::Right);
    m_data.isHorizontalSplit = isHorizontal;
    
    if (direction == DockDirection::Left || direction == DockDirection::Top) {
        AddChild(newNode);
        AddChild(existingNode);
    } else {
        AddChild(existingNode);
        AddChild(newNode);
    }
    
    return newNode;
}

void DockNode::Cleanup() {
    // Remove empty children
    m_children.erase(
        std::remove_if(m_children.begin(), m_children.end(),
            [](const std::shared_ptr<DockNode>& child) {
                return !child || child->IsEmpty();
            }),
        m_children.end());
    
    // If this is a split node with only one child, promote that child
    if (IsSplit() && m_children.size() == 1) {
        auto child = m_children[0];
        m_type = child->m_type;
        m_panels = child->m_panels;
        m_children = child->m_children;
        m_data = child->m_data;
        
        // Update parent pointers for grandchildren
        for (auto& grandchild : m_children) {
            if (grandchild) {
                grandchild->SetParent(shared_from_this());
            }
        }
    }
    
    // Recursively cleanup children
    for (auto& child : m_children) {
        if (child) {
            child->Cleanup();
        }
    }
}

std::shared_ptr<DockNode> DockNode::FindNodeWithPanel(const std::string& panelName) {
    if (IsLeaf() && HasPanel(panelName)) {
        return shared_from_this();
    }
    
    for (auto& child : m_children) {
        if (child) {
            auto result = child->FindNodeWithPanel(panelName);
            if (result) return result;
        }
    }
    
    return nullptr;
}

std::shared_ptr<DockNode> DockNode::FindNodeWithPanel(std::shared_ptr<Panel> panel) {
    if (!panel) return nullptr;
    return FindNodeWithPanel(panel->GetName());
}

} // namespace BGE