#pragma once

#include "../Legacy/TabbedPanel.h"
#include "../Framework/LayoutInfo.h"
#include <unordered_map>
#include <memory>
#include <string>

namespace BGE {

class UnityLayoutManager {
public:
    UnityLayoutManager();
    
    void Initialize();
    void Render();
    
    // Panel management
    void AddPanelToArea(std::shared_ptr<Panel> panel, const std::string& areaName);
    void RemovePanelFromArea(const std::string& panelName, const std::string& areaName);
    void MovePanelToArea(const std::string& panelName, const std::string& fromArea, const std::string& toArea);
    
    // Layout areas
    void UpdateLayout(const LayoutInfo& layoutInfo);
    
    // Unity-style area names
    enum class LayoutArea {
        Left,
        Center,
        Right,
        Bottom
    };
    
    std::string GetAreaName(LayoutArea area) const;

private:
    std::unordered_map<std::string, std::unique_ptr<TabbedPanel>> m_tabbedPanels;
    LayoutInfo m_currentLayout;
    
    // Store panels that are added before areas are created
    std::vector<std::pair<std::shared_ptr<Panel>, std::string>> m_pendingPanels;
    
    void CreateDefaultAreas();
    void UpdateAreaSizes();
    void UpdateLayoutForCurrentScreen();
    void ProcessPendingPanels();
};

} // namespace BGE