#pragma once

#include "Panel.h"
#include "../../Simulation/SimulationWorld.h"
#include "../../Core/Entity.h"

namespace BGE {

class HierarchyPanel : public Panel {
public:
    HierarchyPanel(const std::string& name, SimulationWorld* world);
    
    void Initialize() override;
    void OnRender() override;
    
private:
    void RenderEntityHierarchy();
    void RenderCameraSection();
    void RenderLightsSection();
    void RenderRigidBodiesSection();
    void RenderOtherEntitiesSection();
    
    void RenderEntity(Entity* entity, const std::string& category);
    
    SimulationWorld* m_world;
    Entity* m_selectedEntity = nullptr;
};

} // namespace BGE