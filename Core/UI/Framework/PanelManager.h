#pragma once

#include "Panel.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

namespace BGE {

class PanelManager {
public:
    PanelManager() = default;
    ~PanelManager() = default;
    
    // Register a panel
    template<typename T, typename... Args>
    std::shared_ptr<T> RegisterPanel(const std::string& name, Args&&... args) {
        auto panel = std::make_shared<T>(name, std::forward<Args>(args)...);
        panel->Initialize();
        m_panels.push_back(panel);
        m_panelMap[name] = panel;
        return panel;
    }
    
    // Get panel by name
    std::shared_ptr<Panel> GetPanel(const std::string& name) {
        auto it = m_panelMap.find(name);
        return (it != m_panelMap.end()) ? it->second : nullptr;
    }
    
    // Get panel by type
    template<typename T>
    std::shared_ptr<T> GetPanel() {
        for (auto& panel : m_panels) {
            if (auto typedPanel = std::dynamic_pointer_cast<T>(panel)) {
                return typedPanel;
            }
        }
        return nullptr;
    }
    
    // Render all visible panels
    void RenderAll() {
        for (auto& panel : m_panels) {
            panel->Render();
        }
    }
    
    // Show/hide panels
    void ShowPanel(const std::string& name, bool show = true) {
        if (auto panel = GetPanel(name)) {
            panel->SetVisible(show);
        }
    }
    
    void HidePanel(const std::string& name) {
        ShowPanel(name, false);
    }
    
    void TogglePanel(const std::string& name) {
        if (auto panel = GetPanel(name)) {
            panel->ToggleVisible();
        }
    }
    
    // Shutdown all panels
    void Shutdown() {
        for (auto& panel : m_panels) {
            panel->Shutdown();
        }
        m_panels.clear();
        m_panelMap.clear();
    }
    
    // Get all panels
    const std::vector<std::shared_ptr<Panel>>& GetPanels() const { return m_panels; }
    
private:
    std::vector<std::shared_ptr<Panel>> m_panels;
    std::unordered_map<std::string, std::shared_ptr<Panel>> m_panelMap;
};

} // namespace BGE