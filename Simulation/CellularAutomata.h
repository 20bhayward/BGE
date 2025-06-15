#pragma once

#include "Materials/Material.h"
#include <functional>
#include <array>

namespace BGE {

struct Cell;
class SimulationWorld;

using UpdateRule = std::function<void(SimulationWorld*, int x, int y, float deltaTime)>;

class CellularAutomata {
public:
    explicit CellularAutomata(SimulationWorld* world);
    ~CellularAutomata();
    
    // Core simulation methods
    void Update(float deltaTime);
    void ProcessCell(int x, int y, float deltaTime);
    void ApplyGravity(int x, int y);
    void ApplyLiquidFlow(int x, int y);
    void ApplyGasDispersion(int x, int y);
    void ApplyTemperatureTransfer(int x, int y, float deltaTime);
    
    // Material-specific behaviors
    void ProcessPowder(int x, int y);
    void ProcessLiquid(int x, int y);
    void ProcessGas(int x, int y);
    void ProcessFire(int x, int y);
    
    // Fire type specific behaviors
    void ProcessNormalFire(int x, int y, uint8_t fireLife);
    void ProcessLightning(int x, int y);
    
    // Advanced liquid behaviors
    void ProcessWater(int x, int y, float viscosity, float density);
    void ProcessOil(int x, int y, float viscosity, float density);
    void ProcessPoisonWater(int x, int y, float viscosity, float density);
    void ProcessGenericLiquid(int x, int y, float viscosity, float density);
    void ProcessLiquidNitrogen(int x, int y, float viscosity, float density);
    void ProcessLava(int x, int y, float viscosity, float density);
    void ProcessAcid(int x, int y, float viscosity, float density);
    void ProcessBlood(int x, int y, float viscosity, float density);
    void ProcessQuicksilver(int x, int y, float viscosity, float density);
    
    // Advanced gas behaviors
    void ProcessNitrogen(int x, int y);
    void ProcessSteam(int x, int y);
    void ProcessSmoke(int x, int y);
    void ProcessToxicGas(int x, int y);
    void ProcessCarbonDioxide(int x, int y);
    void ProcessOxygen(int x, int y);
    void ProcessHydrogen(int x, int y);
    void ProcessMethane(int x, int y);
    void ProcessChlorine(int x, int y);
    void ProcessAmmonia(int x, int y);
    void ProcessHelium(int x, int y);
    void ProcessGenericGas(int x, int y, float density);
    
    // New gas processors
    void ProcessArgon(int x, int y);
    void ProcessNeon(int x, int y);
    void ProcessPropane(int x, int y);
    void ProcessAcetylene(int x, int y);
    void ProcessSulfurDioxide(int x, int y);
    void ProcessCarbonMonoxide(int x, int y);
    void ProcessNitrousOxide(int x, int y);
    void ProcessOzone(int x, int y);
    void ProcessFluorine(int x, int y);
    void ProcessXenon(int x, int y);
    
    // Gas movement helpers
    bool TryTurbulentMovement(int x, int y, float upwardBias, float horizontalTurbulence, int attempts = 3);
    
    // Powder physics helpers
    bool TryPowderMove(int fromX, int fromY, int toX, int toY, float density);
    bool ShouldPowderSlide(int x, int y, float angleOfRepose);
    int GetSlideDirection(int x, int y);
    bool IsPowderStable(int x, int y);
    float GetPowderAngleOfRepose(const std::string& materialName);
    float GetPowderCohesion(const std::string& materialName);
    
    // State transitions removed - using direct reactions instead
    void UpdatePowder(int x, int y);
    void UpdateLiquid(int x, int y);
    void UpdateGas(int x, int y);
    void UpdateFire(int x, int y);
    void UpdateStatic(int x, int y);
    
    // Physics calculations
    bool TryMove(int fromX, int fromY, int toX, int toY);
    bool TryFall(int x, int y);
    bool TrySlide(int x, int y);
    bool TryFlow(int x, int y, int direction);
    bool TryRise(int x, int y);
    
    // Reaction processing
    void ProcessReactions(int x, int y, float deltaTime);
    bool TryReaction(int x, int y, const MaterialReaction& reaction, float deltaTime);
    
    // Explosion system
    void CreateExplosion(int centerX, int centerY, float power, float radius);
    bool CanDestroyMaterial(MaterialID material, float explosivePower) const;
    
    // State transitions (melting, boiling, etc.)
    void ProcessStateTransitions(int x, int y);
    MaterialID GetTransitionState(MaterialID material, float temperature);
    
    // Neighbor analysis
    struct NeighborInfo {
        std::array<MaterialID, 8> materials;
        std::array<float, 8> temperatures;
        int emptyCount = 0;
        int liquidCount = 0;
        int gasCount = 0;
        int solidCount = 0;
    };
    
    NeighborInfo AnalyzeNeighbors(int x, int y);
    
    // Optimization features
    void EnableChunking(bool enable) { m_chunkingEnabled = enable; }
    void SetUpdateFrequency(MaterialBehavior behavior, int frequency);
    
private:
    // Helper methods
    bool IsEmpty(int x, int y) const;
    bool CanMove(int fromX, int fromY, int toX, int toY) const;
    bool CanDisplace(MaterialID mover, MaterialID target) const;
    void SwapCells(int x1, int y1, int x2, int y2);
    
    // Physics analysis helpers
    int GetPileHeight(int x, int y) const;
    int GetLiquidColumn(int x, int y) const;
    MaterialID GetWoodMaterialID() const;
    MaterialID GetFireMaterialID() const;
    
    // Density-based movement
    float GetDensity(MaterialID material) const;
    bool ShouldFloat(MaterialID material, MaterialID surroundingMaterial) const;
    
    // Temperature system removed
    
    // Random number generation for probabilistic behaviors
    float Random01();
    bool RandomChance(float probability);
    int RandomDirection(); // -1, 0, or 1
    
    // Update frequency optimization
    bool ShouldUpdate(int x, int y, MaterialBehavior behavior);
    
    SimulationWorld* m_world;
    
    // Optimization settings
    bool m_chunkingEnabled = true;
    std::array<int, 5> m_updateFrequencies = {1, 1, 1, 1, 1}; // Per MaterialBehavior
    
    // Random state (thread-safe)
    thread_local static uint32_t s_randomState;
    
    // Constants for fine-tuning behavior
    static constexpr float LIQUID_FLOW_RATE = 0.8f;
    static constexpr float GAS_DISPERSION_RATE = 0.9f;
    static constexpr float POWDER_SLIDE_ANGLE = 0.7f; // ~35 degrees
    static constexpr float HEAT_TRANSFER_RATE = 0.1f;
    static constexpr int MAX_FALL_VELOCITY = 5;
    static constexpr int MAX_FLOW_VELOCITY = 3;
    
    // Neighbor offsets (8-directional)
    static constexpr std::array<std::pair<int, int>, 8> NEIGHBOR_OFFSETS = {{
        {-1, -1}, {0, -1}, {1, -1},  // Top row
        {-1,  0},          {1,  0},  // Middle row (excluding center)
        {-1,  1}, {0,  1}, {1,  1}   // Bottom row
    }};
    
    // Cardinal directions for flow
    static constexpr std::array<std::pair<int, int>, 4> CARDINAL_OFFSETS = {{
        {0, -1}, {1, 0}, {0, 1}, {-1, 0}  // Up, Right, Down, Left
    }};
};

// Implementations moved to .cpp file to avoid incomplete type issues

inline float CellularAutomata::Random01() {
    // Fast xorshift random number generator
    s_randomState ^= s_randomState << 13;
    s_randomState ^= s_randomState >> 17;
    s_randomState ^= s_randomState << 5;
    return static_cast<float>(s_randomState) / static_cast<float>(UINT32_MAX);
}

inline bool CellularAutomata::RandomChance(float probability) {
    return Random01() < probability;
}

inline int CellularAutomata::RandomDirection() {
    return static_cast<int>(Random01() * 3.0f) - 1; // -1, 0, or 1
}

} // namespace BGE