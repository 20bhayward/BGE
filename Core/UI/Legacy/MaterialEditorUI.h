#pragma once

#include <memory>
#include "../../Input/MaterialTools.h"
#include "../Framework/Panel.h"
#include "../Panels/ProjectSettingsPanel.h"

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
    
    // Panel access
    std::shared_ptr<Panel> GetHierarchyPanel() { return m_hierarchyPanel; }
    std::shared_ptr<Panel> GetInspectorPanel() { return m_inspectorPanel; }
    std::shared_ptr<Panel> GetGameViewPanel() { return m_gameViewportPanel; }
    std::shared_ptr<Panel> GetSceneViewPanel() { return m_sceneViewPanel; }
    std::shared_ptr<Panel> GetSculptingPanel() { return m_sculptingPanel; }
    std::shared_ptr<Panel> GetGamePanel() { return m_gamePanel; }
    std::shared_ptr<Panel> GetAssetBrowserPanel() { return m_assetBrowserPanel; }
    std::shared_ptr<Panel> GetMaterialPalettePanel() { return m_materialPalettePanel; }
    
    // UI state
    bool IsVisible() const { return m_visible; }
    void SetVisible(bool visible) { m_visible = visible; }
    

private:
    void RenderMainMenuBar();
    void SetupDefaultLayout();
    void SetupDockspace();
    void RenderPanels();
    
    MaterialTools* m_materialTools = nullptr;
    SimulationWorld* m_world = nullptr;
    
    // Panel instances (managed by docking system)
    std::shared_ptr<Panel> m_assetBrowserPanel;
    std::shared_ptr<Panel> m_hierarchyPanel;
    std::shared_ptr<Panel> m_gameViewportPanel;  // Keep for backward compatibility
    std::shared_ptr<Panel> m_sceneViewPanel;
    std::shared_ptr<Panel> m_sculptingPanel;
    std::shared_ptr<Panel> m_gamePanel;
    std::shared_ptr<Panel> m_inspectorPanel;
    std::shared_ptr<Panel> m_materialPalettePanel;
    
    // Standalone windows (not docked)
    std::unique_ptr<ProjectSettingsPanel> m_projectSettingsPanel;
    
    // UI state
    bool m_visible = true;
    bool m_showDemoWindow = false;
};

} // namespace BGE