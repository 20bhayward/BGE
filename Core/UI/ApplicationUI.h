#pragma once

#include <memory>
#include "Legacy/MaterialEditorUI.h"

namespace BGE {

class SimulationWorld;
class MaterialTools;

/**
 * Main application UI coordinator
 * Currently delegates to MaterialEditorUI but provides a cleaner interface
 * TODO: Eventually replace with proper decoupled managers
 */
class ApplicationUI {
public:
    ApplicationUI();
    ~ApplicationUI() = default;
    
    // Lifecycle
    bool Initialize(MaterialTools* tools, SimulationWorld* world);
    void Shutdown();
    void Render();
    
    // Panel access (delegates to MaterialEditorUI)
    std::shared_ptr<Panel> GetPanel(const std::string& name) const;
    void ShowPanel(const std::string& name, bool show = true);
    void TogglePanel(const std::string& name);
    
    // UI state
    bool IsVisible() const { return m_visible; }
    void SetVisible(bool visible) { m_visible = visible; }
    
private:
    // Core references
    MaterialTools* m_materialTools = nullptr;
    SimulationWorld* m_world = nullptr;
    
    // Delegate to existing UI system for now
    std::unique_ptr<MaterialEditorUI> m_materialEditorUI;
    
    // UI state
    bool m_visible = true;
    bool m_initialized = false;
};

} // namespace BGE