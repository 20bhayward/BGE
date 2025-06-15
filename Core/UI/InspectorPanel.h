#pragma once

#include "Panel.h"
#include "../Input/MaterialTools.h"
#include "../../Simulation/SimulationWorld.h"
#include "../../Renderer/Renderer.h"
#include "../../Renderer/PostProcessor.h"
#include "../../Renderer/ParticleSystem.h"
#include "../../Renderer/PixelCamera.h"

namespace BGE {

class InspectorPanel : public Panel {
public:
    InspectorPanel(const std::string& name, MaterialTools* tools, SimulationWorld* world);
    
    void Initialize() override;
    void OnRender() override;
    
private:
    void RenderMaterialInfo();
    void RenderSimulationInfo();
    void RenderCameraAndEffects();
    
    MaterialTools* m_tools;
    SimulationWorld* m_world;
};

} // namespace BGE