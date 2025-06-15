#pragma once

#include "Panel.h"
#include "../../Simulation/SimulationWorld.h"
#include "../Input/MaterialTools.h"

namespace BGE {

class DebugToolbarPanel : public Panel {
public:
    DebugToolbarPanel(const std::string& name, SimulationWorld* world, MaterialTools* tools);
    
    void Initialize() override;
    void OnRender() override;
    
private:
    void RenderSimulationControls();
    void RenderSimulationSettings();
    void RenderDebugControls();
    void RenderPerformanceInfo();
    
    SimulationWorld* m_world;
    MaterialTools* m_tools;
    
    // Debug settings
    bool m_showGrid = false;
    bool m_showStats = true;
    bool m_showDebugInfo = false;
    bool m_showWireframe = false;
    
    // Simulation settings
    bool m_showWorldSizeDialog = false;
};

} // namespace BGE