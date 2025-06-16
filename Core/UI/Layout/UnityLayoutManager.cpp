#include "UnityLayoutManager.h"
#include <imgui.h>
#include <algorithm>
#include <iostream>

namespace BGE {

UnityLayoutManager::UnityLayoutManager() {
}

void UnityLayoutManager::Initialize() {
    CreateDefaultAreas();
}

void UnityLayoutManager::CreateDefaultAreas() {
    // Create default Unity-style areas with dynamic sizing
    ImGuiIO& io = ImGui::GetIO();
    float screenWidth = io.DisplaySize.x;
    float screenHeight = io.DisplaySize.y;
    
    std::cout << "UnityLayoutManager::CreateDefaultAreas() - Screen size: " << screenWidth << "x" << screenHeight << std::endl;
    
    // Don't create areas if ImGui isn't ready yet
    if (screenWidth <= 0 || screenHeight <= 0) {
        std::cout << "  Skipping area creation - invalid screen size" << std::endl;
        return;
    }
    
    // Calculate responsive areas
    float leftWidth = std::max(200.0f, screenWidth * 0.2f);
    float rightWidth = std::max(200.0f, screenWidth * 0.25f);
    float topToolbarHeight = 35.0f;
    float bottomHeight = std::max(120.0f, screenHeight * 0.2f);
    float menuBarHeight = 20.0f;
    
    // Create panels with dynamic sizing
    m_tabbedPanels["Left"] = std::make_unique<TabbedPanel>("Left", 
        ImVec4(0, menuBarHeight, leftWidth, screenHeight - menuBarHeight - bottomHeight));
    
    m_tabbedPanels["TopToolbar"] = std::make_unique<TabbedPanel>("TopToolbar", 
        ImVec4(leftWidth, menuBarHeight, screenWidth - leftWidth - rightWidth, topToolbarHeight));
    
    m_tabbedPanels["Center"] = std::make_unique<TabbedPanel>("Center", 
        ImVec4(leftWidth, menuBarHeight + topToolbarHeight, 
               screenWidth - leftWidth - rightWidth, 
               screenHeight - menuBarHeight - topToolbarHeight - bottomHeight));
    
    m_tabbedPanels["Right"] = std::make_unique<TabbedPanel>("Right", 
        ImVec4(screenWidth - rightWidth, menuBarHeight, rightWidth, screenHeight - menuBarHeight - bottomHeight));
    
    m_tabbedPanels["Bottom"] = std::make_unique<TabbedPanel>("Bottom", 
        ImVec4(0, screenHeight - bottomHeight, screenWidth, bottomHeight));
    
    // Process any panels that were added before areas were created
    ProcessPendingPanels();
}

void UnityLayoutManager::UpdateLayout(const LayoutInfo& layoutInfo) {
    m_currentLayout = layoutInfo;
    UpdateAreaSizes();
}

void UnityLayoutManager::UpdateAreaSizes() {
    if (m_tabbedPanels.find("Left") != m_tabbedPanels.end()) {
        m_tabbedPanels["Left"]->SetArea(m_currentLayout.leftArea);
    }
    if (m_tabbedPanels.find("TopToolbar") != m_tabbedPanels.end()) {
        m_tabbedPanels["TopToolbar"]->SetArea(m_currentLayout.topToolbarArea);
    }
    if (m_tabbedPanels.find("Center") != m_tabbedPanels.end()) {
        m_tabbedPanels["Center"]->SetArea(m_currentLayout.centerArea);
    }
    if (m_tabbedPanels.find("Right") != m_tabbedPanels.end()) {
        m_tabbedPanels["Right"]->SetArea(m_currentLayout.rightArea);
    }
    if (m_tabbedPanels.find("Bottom") != m_tabbedPanels.end()) {
        m_tabbedPanels["Bottom"]->SetArea(m_currentLayout.bottomArea);
    }
}

void UnityLayoutManager::Render() {
    // Update layout based on current screen size
    UpdateLayoutForCurrentScreen();
    
    static int renderCount = 0;
    if (renderCount++ < 5) { // Debug first 5 renders
        std::cout << "UnityLayoutManager::Render() - Rendering " << m_tabbedPanels.size() << " tabbed panels" << std::endl;
    }
    
    for (auto& [name, tabbedPanel] : m_tabbedPanels) {
        if (tabbedPanel && !tabbedPanel->IsEmpty()) {
            if (renderCount <= 5) {
                std::cout << "  Rendering non-empty panel: " << name << std::endl;
            }
            tabbedPanel->Render();
        } else if (renderCount <= 5) {
            std::cout << "  Skipping panel: " << name << " (empty: " << (tabbedPanel ? tabbedPanel->IsEmpty() : true) << ")" << std::endl;
        }
    }
}

void UnityLayoutManager::AddPanelToArea(std::shared_ptr<Panel> panel, const std::string& areaName) {
    auto it = m_tabbedPanels.find(areaName);
    if (it != m_tabbedPanels.end() && panel) {
        std::cout << "UnityLayoutManager::AddPanelToArea() - Adding panel '" << panel->GetName() << "' to area '" << areaName << "'" << std::endl;
        it->second->AddPanel(panel);
    } else {
        std::cout << "UnityLayoutManager::AddPanelToArea() - Deferring panel '" << (panel ? panel->GetName() : "null") << "' for area '" << areaName << "'" << std::endl;
        // Store panel for later addition when areas are ready
        m_pendingPanels.emplace_back(panel, areaName);
        
        if (it == m_tabbedPanels.end()) {
            std::cout << "  Area '" << areaName << "' not found. Will add when areas are created." << std::endl;
        }
    }
}

void UnityLayoutManager::RemovePanelFromArea(const std::string& panelName, const std::string& areaName) {
    auto it = m_tabbedPanels.find(areaName);
    if (it != m_tabbedPanels.end()) {
        it->second->RemovePanel(panelName);
    }
}

void UnityLayoutManager::MovePanelToArea(const std::string& panelName, const std::string& fromArea, const std::string& toArea) {
    // Find the panel in the source area
    auto fromIt = m_tabbedPanels.find(fromArea);
    auto toIt = m_tabbedPanels.find(toArea);
    
    if (fromIt != m_tabbedPanels.end() && toIt != m_tabbedPanels.end()) {
        // Find and move the panel (simplified - in real implementation would need to extract the panel)
        // For now, just remove from source and the external code should re-add to destination
        fromIt->second->RemovePanel(panelName);
    }
}

std::string UnityLayoutManager::GetAreaName(LayoutArea area) const {
    switch (area) {
        case LayoutArea::Left: return "Left";
        case LayoutArea::Center: return "Center";
        case LayoutArea::Right: return "Right";
        case LayoutArea::Bottom: return "Bottom";
        default: return "Center";
    }
}

void UnityLayoutManager::UpdateLayoutForCurrentScreen() {
    ImGuiIO& io = ImGui::GetIO();
    static float lastScreenWidth = -1;
    static float lastScreenHeight = -1;
    static bool areasCreated = false;
    
    // Create areas if we haven't yet or if screen size changed
    bool needsUpdate = (io.DisplaySize.x != lastScreenWidth || io.DisplaySize.y != lastScreenHeight);
    bool hasValidSize = (io.DisplaySize.x > 0 && io.DisplaySize.y > 0);
    
    if (needsUpdate && hasValidSize) {
        lastScreenWidth = io.DisplaySize.x;
        lastScreenHeight = io.DisplaySize.y;
        CreateDefaultAreas(); // Recreate areas with new dimensions
        areasCreated = true;
    } else if (!areasCreated && hasValidSize) {
        // Force creation if we never created areas due to invalid size
        CreateDefaultAreas();
        areasCreated = true;
        lastScreenWidth = io.DisplaySize.x;
        lastScreenHeight = io.DisplaySize.y;
    }
}

void UnityLayoutManager::ProcessPendingPanels() {
    std::cout << "UnityLayoutManager::ProcessPendingPanels() - Processing " << m_pendingPanels.size() << " pending panels" << std::endl;
    
    for (const auto& [panel, areaName] : m_pendingPanels) {
        auto it = m_tabbedPanels.find(areaName);
        if (it != m_tabbedPanels.end() && panel) {
            std::cout << "  Adding deferred panel '" << panel->GetName() << "' to area '" << areaName << "'" << std::endl;
            it->second->AddPanel(panel);
        }
    }
    
    // Clear pending panels after processing
    m_pendingPanels.clear();
}

} // namespace BGE