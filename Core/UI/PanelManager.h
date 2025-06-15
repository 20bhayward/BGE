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
    T* RegisterPanel(const std::string& name, Args&&... args) {
        auto panel = std::make_unique<T>(name, std::forward<Args>(args)...);
        T* ptr = panel.get();
        panel->Initialize();
        m_panels.push_back(std::move(panel));
        m_panelMap[name] = ptr;
        return ptr;
    }
    
    // Get panel by name
    Panel* GetPanel(const std::string& name) {
        auto it = m_panelMap.find(name);
        return (it != m_panelMap.end()) ? it->second : nullptr;
    }
    
    // Get panel by type
    template<typename T>
    T* GetPanel() {
        for (auto& panel : m_panels) {
            if (T* typedPanel = dynamic_cast<T*>(panel.get())) {
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
        if (Panel* panel = GetPanel(name)) {
            panel->SetVisible(show);
        }
    }
    
    void HidePanel(const std::string& name) {
        ShowPanel(name, false);
    }
    
    void TogglePanel(const std::string& name) {
        if (Panel* panel = GetPanel(name)) {
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
    const std::vector<std::unique_ptr<Panel>>& GetPanels() const { return m_panels; }
    
private:
    std::vector<std::unique_ptr<Panel>> m_panels;
    std::unordered_map<std::string, Panel*> m_panelMap;
};

} // namespace BGE