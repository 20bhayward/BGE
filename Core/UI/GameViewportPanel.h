#pragma once

#include "Panel.h"
#include "../../Simulation/SimulationWorld.h"
#include "../Input/MaterialTools.h"
#include "../Entity.h"

namespace BGE {

class GameViewportPanel : public Panel {
public:
    GameViewportPanel(const std::string& name, SimulationWorld* world, MaterialTools* tools);
    
    void Initialize() override;
    void OnRender() override;
    
    // Get the viewport bounds for rendering
    void GetViewportBounds(float& x, float& y, float& width, float& height) const {
        x = m_viewportX;
        y = m_viewportY;
        width = m_viewportWidth;
        height = m_viewportHeight;
    }
    
    bool IsHovered() const { return m_isHovered; }
    bool IsFocused() const { return m_isFocused; }
    
private:
    void RenderViewportToolbar();
    void RenderGameContent();
    void HandleCameraInput(float mouseX, float mouseY);
    void HandleKeyboardInput();
    void HandleImageInput(ImVec2 cursorPos, ImVec2 contentRegion);
    void HandleMaterialDragAndDrop(ImVec2 mousePos, ImVec2 contentRegion);
    void ApplyMaterialToEntity(EntityID entityId, const std::string& materialPath);
    
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
    
    // Overlay settings
    bool m_showGrid = false;
    bool m_showToolbar = true;
};

} // namespace BGE