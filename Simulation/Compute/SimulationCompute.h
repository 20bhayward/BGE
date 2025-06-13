#pragma once

#ifdef BGE_COMPUTE_SUPPORT

#include "../SimulationWorld.h"

namespace BGE {

class SimulationCompute {
public:
    SimulationCompute() = default;
    ~SimulationCompute() = default;
    
    bool Initialize();
    void Shutdown();
    
    // GPU-accelerated simulation methods
    void UpdateCellularAutomataGPU(SimulationWorld* world, float deltaTime);
    void UpdatePhysicsGPU(SimulationWorld* world, float deltaTime);
    
private:
    bool m_initialized = false;
    // TODO: Add Vulkan compute pipeline resources
};

} // namespace BGE

#endif // BGE_COMPUTE_SUPPORT