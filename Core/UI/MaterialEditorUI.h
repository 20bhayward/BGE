#pragma once

#include "UISystem.h"
#include "../Input/MaterialTools.h"

namespace BGE {

class SimulationWorld;

class MaterialEditorUI {
public:
    MaterialEditorUI();
    ~MaterialEditorUI() = default;
    
    void Initialize(MaterialTools* tools, SimulationWorld* world);
    void Render();
    
    // UI state
    bool IsVisible() const { return m_visible; }
    void SetVisible(bool visible) { m_visible = visible; }
    
    // Panel visibility
    void ShowMaterialPalette(bool show) { m_showMaterialPalette = show; }
    void ShowSimulationControls(bool show) { m_showSimulationControls = show; }
    void ShowBrushSettings(bool show) { m_showBrushSettings = show; }
    void ShowStatusPanel(bool show) { m_showStatusPanel = show; }
    void ShowMaterialInfo(bool show) { m_showMaterialInfo = show; }

private:
    void RenderMaterialPalette();
    void RenderSimulationControls();
    void RenderBrushSettings();
    void RenderStatusPanel();
    void RenderMaterialInfo();
    void RenderMainMenuBar();
    
    MaterialTools* m_materialTools = nullptr;
    SimulationWorld* m_world = nullptr;
    
    // UI state
    bool m_visible = true;
    bool m_showMaterialPalette = true;
    bool m_showSimulationControls = true;
    bool m_showBrushSettings = true;
    bool m_showStatusPanel = true;
    bool m_showMaterialInfo = true;
    bool m_showDemoWindow = false;
    
    // Layout
    float m_paletteWidth = 200.0f;
    float m_controlsHeight = 120.0f;
    
    // Colors for UI
    static constexpr float PANEL_ALPHA = 0.95f;
};

} // namespace BGE