#pragma once

#include "Panel.h"
#include "../../Simulation/SimulationWorld.h"
#include "../Input/MaterialTools.h"

namespace BGE {

class SceneViewPanel : public Panel {
public:
    SceneViewPanel(const std::string& name, SimulationWorld* world, MaterialTools* tools);
    
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
    void RenderToolbar();
    void RenderViewport();
    void RenderOverlay();
    
    SimulationWorld* m_world;
    MaterialTools* m_tools;
    
    // Viewport info
    float m_viewportX = 0;
    float m_viewportY = 0;
    float m_viewportWidth = 800;
    float m_viewportHeight = 600;
    bool m_isHovered = false;
    bool m_isFocused = false;
    
    // Toolbar state
    bool m_showGrid = false;
    bool m_showStats = true;
    bool m_showDebugInfo = false;
};

} // namespace BGE