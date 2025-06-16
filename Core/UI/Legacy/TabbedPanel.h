#pragma once

#include "../Framework/Panel.h"
#include <vector>
#include <memory>
#include <string>

namespace BGE {

class TabbedPanel {
public:
    TabbedPanel(const std::string& name, ImVec4 area);
    
    void AddPanel(std::shared_ptr<Panel> panel);
    void RemovePanel(const std::string& panelName);
    void SetActivePanel(const std::string& panelName);
    
    void Render();
    
    const std::string& GetName() const { return m_name; }
    bool IsEmpty() const { return m_panels.empty(); }
    
    // Unity-style tab management
    bool HasPanel(const std::string& panelName) const;
    void SetArea(ImVec4 area) { m_area = area; }
    const ImVec4& GetArea() const { return m_area; }

private:
    std::string m_name;
    ImVec4 m_area; // x, y, width, height
    std::vector<std::shared_ptr<Panel>> m_panels;
    int m_activeTab = 0;
    
    void RenderTabBar();
    void RenderActivePanel();
};

} // namespace BGE