#pragma once

#include <memory>
#include <vector>
#include <atomic>
#include "Materials/Material.h"
#include "World/ChunkManager.h"

namespace BGE {

class MaterialSystem;
class PhysicsWorld;
class ThreadPool;
class CellularAutomata;

// Effect layers for enhanced visual realism
enum class EffectLayer : uint8_t {
    None = 0,
    Burning = 1,        // Orange/red glow, flickering
    Freezing = 2,       // Blue tint, ice crystals
    Electrified = 3,    // Blue-white sparks, lightning
    Bloodied = 4,       // Red stains
    Blackened = 5,      // Soot/explosion damage
    Corroding = 6,      // Green acid effects
    Crystallizing = 7,  // Crystal formation patterns
    Glowing = 8         // General luminescence
};

struct Cell {
    MaterialID material = MATERIAL_EMPTY;
    float temperature = 20.0f;  // Celsius - still used for some effects
    uint8_t velocity_x = 0;     // Velocity for liquids/gases
    uint8_t velocity_y = 0;
    uint8_t life = 0;           // For temporary materials (fire, etc.)
    uint8_t flags = 0;          // Bit flags for various states
    
    // New effect layer system
    EffectLayer effectLayer = EffectLayer::None;
    uint8_t effectIntensity = 0;    // 0-255 intensity of effect
    uint8_t effectTimer = 0;        // Countdown for temporary effects
    uint8_t effectData = 0;         // Extra data for complex effects
};

class SimulationWorld {
public:
    explicit SimulationWorld(uint32_t width, uint32_t height);
    ~SimulationWorld();
    
    // Core simulation
    void Update(float deltaTime);
    void Reset();
    void Clear();
    
    // World properties
    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    bool IsValidPosition(int x, int y) const;
    
    // Cell access
    const Cell& GetCell(int x, int y) const;
    void SetMaterial(int x, int y, MaterialID material);
    void SetTemperature(int x, int y, float temperature);
    
    // Effect layer system
    void SetEffect(int x, int y, EffectLayer effect, uint8_t intensity, uint8_t duration = 255);
    void ClearEffect(int x, int y);
    EffectLayer GetEffect(int x, int y) const;
    uint8_t GetEffectIntensity(int x, int y) const;
    
    // Next grid access (for double buffering)
    void SetNextMaterial(int x, int y, MaterialID material);
    void SetNextTemperature(int x, int y, float temperature);
    Cell& GetNextCell(int x, int y);
    
    MaterialID GetMaterial(int x, int y) const;
    float GetTemperature(int x, int y) const;
    
    // Bulk operations
    void FillRegion(int x1, int y1, int x2, int y2, MaterialID material);
    void FillCircle(int centerX, int centerY, int radius, MaterialID material);
    
    // Systems access
    MaterialSystem* GetMaterialSystem() const { return m_materialSystem.get(); }
    PhysicsWorld* GetPhysicsWorld() const { return m_physicsWorld.get(); }
    ChunkManager* GetChunkManager() const { return m_chunkManager.get(); }
    
    // Rendering support
    const uint8_t* GetPixelData() const { return m_pixelBuffer.data(); }
    bool IsRegionDirty(int x, int y, int width, int height) const;
    void MarkRegionClean(int x, int y, int width, int height);
    
    // Simulation control
    void Play() { m_paused = false; }
    void Pause() { m_paused = true; }
    void TogglePause() { m_paused = !m_paused; }
    bool IsPaused() const { return m_paused; }
    void Step() { m_stepOnce = true; } // Advance one frame while paused
    void Stop() { Pause(); Reset(); }
    
    // Performance settings
    void SetMultithreading(bool enabled) { m_multithreading = enabled; }
    void SetMaxThreads(uint32_t threads);
    void SetSimulationSpeed(float speed) { m_simulationSpeed = speed; }
    
    // Debug information
    uint64_t GetUpdateCount() const { return m_updateCount; }
    float GetLastUpdateTime() const { return m_lastUpdateTime; }
    uint32_t GetActiveCells() const { return m_activeCells; }

private:
    // Core update methods
    void UpdateCellularAutomata(float deltaTime);
    void UpdateTemperature(float deltaTime);
    void UpdateReactions(float deltaTime);
    void UpdateEffects(float deltaTime);
    void UpdatePhysics(float deltaTime);
    void UpdatePixelBuffer();
    
    // Simulation rules
    void ProcessCell(int x, int y, float deltaTime);
    void ProcessPowder(int x, int y);
    void ProcessLiquid(int x, int y);
    void ProcessGas(int x, int y);
    void ProcessFire(int x, int y);
    
    // Helper methods
    bool TryMove(int fromX, int fromY, int toX, int toY);
    bool CanDisplace(MaterialID mover, MaterialID target) const;
    void SwapCells(int x1, int y1, int x2, int y2);
    uint32_t MaterialToColor(MaterialID material, float temperature, int x, int y) const;
    uint32_t ApplyVisualPattern(uint32_t baseColor, const VisualProperties& props, int x, int y) const;
    uint32_t BlendEffectLayer(uint32_t baseColor, EffectLayer effect, uint8_t intensity) const;
    
    // Threading support
    void UpdateChunkParallel(int chunkIndex, float deltaTime);
    void ProcessCellRange(int startX, int startY, int endX, int endY, float deltaTime);
    
    // World dimensions
    uint32_t m_width, m_height;
    
    // Double buffering for thread safety
    std::vector<Cell> m_currentGrid;
    std::vector<Cell> m_nextGrid;
    std::atomic<bool> m_swapBuffers{false};
    
    // Rendering buffer (RGBA)
    std::vector<uint8_t> m_pixelBuffer;
    std::vector<bool> m_dirtyRegions;
    
    // Systems
    std::unique_ptr<MaterialSystem> m_materialSystem;
    std::unique_ptr<PhysicsWorld> m_physicsWorld;
    std::unique_ptr<ChunkManager> m_chunkManager;
    std::unique_ptr<ThreadPool> m_threadPool;
    std::unique_ptr<CellularAutomata> m_cellularAutomata;
    
    // Performance tracking
    std::atomic<uint64_t> m_updateCount{0};
    std::atomic<float> m_lastUpdateTime{0.0f};
    std::atomic<uint32_t> m_activeCells{0};
    
    // Settings
    bool m_multithreading = true;
    float m_simulationSpeed = 0.5f;
    uint32_t m_maxThreads = 0; // 0 = auto-detect
    
    // Simulation control
    bool m_paused = false;
    bool m_stepOnce = false;
    
    // Constants
    static constexpr float GRAVITY = 9.81f;
    static constexpr float TEMPERATURE_DIFFUSION = 0.1f;
    static constexpr int CHUNK_SIZE = 64;
};

inline int CoordToIndex(int x, int y, int width) {
    return y * width + x;
}

inline void IndexToCoord(int index, int width, int& x, int& y) {
    x = index % width;
    y = index / width;
}

} // namespace BGE