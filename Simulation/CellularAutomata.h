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
    
    // Temperature effects
    void DiffuseHeat(int x, int y, float deltaTime);
    void ApplyThermalEffects(int x, int y, float temperature);
    
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