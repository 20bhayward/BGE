#pragma once

#include "../Framework/Panel.h"
#include "../../../Simulation/SimulationWorld.h"

namespace BGE {

class GamePanel : public Panel {
public:
    GamePanel(const std::string& name, SimulationWorld* world);
    
    void Initialize() override;
    void OnRender() override;
    
    bool IsHovered() const { return m_isHovered; }
    bool IsFocused() const { return m_isFocused; }
    
private:
    void RenderGameToolbar();
    void RenderGameContent();
    void HandleGameInput(ImVec2 cursorPos, ImVec2 contentRegion);
    
    // Optimized rendering methods
    void RenderOptimizedWorld();
    void CalculateVisibleBounds(int& startX, int& startY, int& endX, int& endY);
    int CalculateLODStep() const;
    void RenderStats();
    
    SimulationWorld* m_world;
    
    // Viewport info
    float m_viewportX = 0;
    float m_viewportY = 0;
    float m_viewportWidth = 800;
    float m_viewportHeight = 600;
    bool m_isHovered = false;
    bool m_isFocused = false;
    
    // Game state
    bool m_isPlaying = false;
    bool m_showStats = false;
    bool m_fullscreen = false;
};

} // namespace BGE