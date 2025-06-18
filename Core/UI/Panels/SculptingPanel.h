#pragma once

#include "../Framework/Panel.h"
#include "../../../Simulation/SimulationWorld.h"
#include "../../Input/MaterialTools.h"

namespace BGE {

class SculptingPanel : public Panel {
public:
    SculptingPanel(const std::string& name, SimulationWorld* world, MaterialTools* tools);
    
    void Initialize() override;
    void OnRender() override;
    
    bool IsHovered() const { return m_isHovered; }
    bool IsFocused() const { return m_isFocused; }
    
private:
    void RenderSculptingToolbar();
    void RenderSculptingContent();
    void HandleCameraInput(float mouseX, float mouseY);
    void HandleMaterialInput(ImVec2 cursorPos, ImVec2 contentRegion);
    void HandleMaterialDragAndDrop(ImVec2 /*mousePos*/, ImVec2 /*contentRegion*/);
    
    SimulationWorld* m_world;
    MaterialTools* m_tools;
    
    // Viewport info
    float m_viewportX = 0;
    float m_viewportY = 0;
    float m_viewportWidth = 800;
    float m_viewportHeight = 600;
    bool m_isHovered = false;
    bool m_isFocused = false;
    
    // Camera controls
    bool m_cameraMode = false;
    bool m_dragging = false;
    float m_lastMouseX = 0;
    float m_lastMouseY = 0;
    
    // Sculpting settings
    bool m_showGrid = true;
    bool m_showMaterialPreview = true;
    bool m_continuousSculpt = false;
};

} // namespace BGE