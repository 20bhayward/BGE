#include "TabbedPanel.h"
#include <imgui.h>
#include <algorithm>

namespace BGE {

TabbedPanel::TabbedPanel(const std::string& name, ImVec4 area)
    : m_name(name), m_area(area) {
}

void TabbedPanel::AddPanel(std::shared_ptr<Panel> panel) {
    if (panel && !HasPanel(panel->GetName())) {
        m_panels.push_back(panel);
    }
}

void TabbedPanel::RemovePanel(const std::string& panelName) {
    auto it = std::find_if(m_panels.begin(), m_panels.end(),
        [&panelName](const std::shared_ptr<Panel>& panel) {
            return panel->GetName() == panelName;
        });
    
    if (it != m_panels.end()) {
        int index = static_cast<int>(std::distance(m_panels.begin(), it));
        m_panels.erase(it);
        
        // Adjust active tab if necessary
        if (m_activeTab >= index && m_activeTab > 0) {
            m_activeTab--;
        }
        if (m_activeTab >= static_cast<int>(m_panels.size()) && !m_panels.empty()) {
            m_activeTab = static_cast<int>(m_panels.size()) - 1;
        }
        if (m_panels.empty()) m_activeTab = 0;
    }
}

void TabbedPanel::SetActivePanel(const std::string& panelName) {
    for (size_t i = 0; i < m_panels.size(); ++i) {
        if (m_panels[i]->GetName() == panelName) {
            m_activeTab = static_cast<int>(i);
            break;
        }
    }
}

bool TabbedPanel::HasPanel(const std::string& panelName) const {
    return std::any_of(m_panels.begin(), m_panels.end(),
        [&panelName](const std::shared_ptr<Panel>& panel) {
            return panel->GetName() == panelName;
        });
}

void TabbedPanel::Render() {
    if (m_panels.empty()) return;
    
    // Set window position and size based on area
    ImGui::SetNextWindowPos(ImVec2(m_area.x, m_area.y));
    ImGui::SetNextWindowSize(ImVec2(m_area.z, m_area.w));
    
    // Make panels resizable by allowing resize but not move (for now)
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | 
                            ImGuiWindowFlags_NoTitleBar;
    
    // Use a unique window name
    std::string windowName = "##TabbedPanel_" + m_name;
    
    if (ImGui::Begin(windowName.c_str(), nullptr, flags)) {
        // Update area if window was resized
        ImVec2 windowSize = ImGui::GetWindowSize();
        ImVec2 windowPos = ImGui::GetWindowPos();
        m_area = ImVec4(windowPos.x, windowPos.y, windowSize.x, windowSize.y);
        
        // If only one panel, render it directly without tabs
        if (m_panels.size() == 1) {
            RenderActivePanel();
        } else {
            // Multiple panels, show tabs
            RenderTabBar();
            ImGui::Separator();
            RenderActivePanel();
        }
    }
    ImGui::End();
}

void TabbedPanel::RenderTabBar() {
    if (m_panels.empty()) return;
    
    // Custom tab bar that looks like Unity
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12, 6));
    
    for (size_t i = 0; i < m_panels.size(); ++i) {
        if (i > 0) ImGui::SameLine();
        
        bool isActive = (m_activeTab == static_cast<int>(i));
        if (isActive) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        }
        
        if (ImGui::Button(m_panels[i]->GetName().c_str())) {
            m_activeTab = static_cast<int>(i);
        }
        
        ImGui::PopStyleColor(3);
    }
    
    ImGui::PopStyleVar(2);
}

void TabbedPanel::RenderActivePanel() {
    if (m_activeTab >= 0 && m_activeTab < static_cast<int>(m_panels.size())) {
        auto& activePanel = m_panels[m_activeTab];
        if (activePanel && activePanel->IsVisible()) {
            activePanel->OnRender();
        }
    }
}

} // namespace BGE