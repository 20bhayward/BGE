#pragma once

#include "../Core/ISystem.h"
#include "../Core/ServiceLocator.h"
#include "../Simulation/SimulationWorld.h"
#include <memory>

namespace BGE {

// Forward declarations for AI components that will be implemented
class Pathfinder;
class BehaviorTree;

class AISystem : public ISystem {
public:
    AISystem();
    virtual ~AISystem();

    bool Initialize();
    void Shutdown();
    void Update(float deltaTime) override;
    const char* GetName() const override { return "AISystem"; }

    // AI Framework methods (to be implemented with actual AI task)
    std::shared_ptr<Pathfinder> GetPathfinder() const { return m_pathfinder; }
    void SetWorld(std::shared_ptr<SimulationWorld> world) { m_world = world; }

private:
    std::shared_ptr<Pathfinder> m_pathfinder;
    std::shared_ptr<SimulationWorld> m_world;
};

} // namespace BGE