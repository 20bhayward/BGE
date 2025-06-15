#pragma once

#include <memory>
#include "../Input/MaterialTools.h"
#include "UnityLayoutManager.h"
#include "Panel.h"

// Forward declarations
namespace BGE {
    class UISystem;
}

namespace BGE {

class SimulationWorld;

class MaterialEditorUI {
public:
    MaterialEditorUI();
    ~MaterialEditorUI() = default;
    
    void Initialize(MaterialTools* tools, SimulationWorld* world);
    void Shutdown();
    void Render();
    
    // Layout management
    UnityLayoutManager& GetLayoutManager() { return m_layoutManager; }
    
    // UI state
    bool IsVisible() const { return m_visible; }
    void SetVisible(bool visible) { m_visible = visible; }
    

private:
    void RenderMainMenuBar();
    void SetupDefaultLayout();
    
    MaterialTools* m_materialTools = nullptr;
    SimulationWorld* m_world = nullptr;
    
    // Unity-style layout management
    UnityLayoutManager m_layoutManager;
    
    // Panel instances
    std::shared_ptr<Panel> m_assetBrowserPanel;
    std::shared_ptr<Panel> m_hierarchyPanel;
    std::shared_ptr<Panel> m_debugToolbarPanel;
    std::shared_ptr<Panel> m_gameViewportPanel;
    std::shared_ptr<Panel> m_inspectorPanel;
    std::shared_ptr<Panel> m_materialPalettePanel;
    
    // UI state
    bool m_visible = true;
    bool m_showDemoWindow = false;
};

} // namespace BGE