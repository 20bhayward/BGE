#pragma once

#include <vector>
#include <atomic>
#include <bitset>

namespace BGE {

constexpr int CHUNK_SIZE = 64;
constexpr int CHUNK_AREA = CHUNK_SIZE * CHUNK_SIZE;

enum class ChunkState : uint8_t {
    Inactive,   // No active particles, skip updates
    Active,     // Has active particles, needs updating
    Dirty,      // Changed this frame, needs rendering update
    Sleeping    // Temporarily inactive but may wake up
};

class Chunk {
public:
    Chunk(int chunkX, int chunkY, int worldOffsetX, int worldOffsetY);
    
    // State management
    ChunkState GetState() const { return m_state; }
    void SetState(ChunkState state) { m_state = state; }
    
    bool IsActive() const { return m_state == ChunkState::Active; }
    bool IsDirty() const { return m_state == ChunkState::Dirty; }
    bool IsSleeping() const { return m_state == ChunkState::Sleeping; }
    
    // Activity tracking
    void MarkActive() { m_state = ChunkState::Active; m_sleepTimer = 0; }
    void MarkDirty() { m_state = ChunkState::Dirty; }
    void MarkClean() { if (m_state == ChunkState::Dirty) m_state = ChunkState::Active; }
    
    // Update management
    void Update(float deltaTime);
    bool ShouldUpdate() const;
    void IncrementSleepTimer() { ++m_sleepTimer; }
    void ResetSleepTimer() { m_sleepTimer = 0; }
    
    // Position info
    int GetChunkX() const { return m_chunkX; }
    int GetChunkY() const { return m_chunkY; }
    int GetWorldOffsetX() const { return m_worldOffsetX; }
    int GetWorldOffsetY() const { return m_worldOffsetY; }
    
    // Activity regions for optimization
    void SetActiveRegion(int minX, int minY, int maxX, int maxY);
    void GetActiveRegion(int& minX, int& minY, int& maxX, int& maxY) const;
    bool HasActiveRegion() const { return m_hasActiveRegion; }
    void ClearActiveRegion() { m_hasActiveRegion = false; }
    
    // Neighbor awareness for edge effects
    void SetNeighborActivity(int direction, bool active);
    bool GetNeighborActivity(int direction) const;
    
    // Performance metrics
    uint32_t GetUpdateCount() const { return m_updateCount; }
    float GetLastUpdateTime() const { return m_lastUpdateTime; }
    uint32_t GetActiveCellCount() const { return m_activeCellCount; }
    
    // Thread safety
    bool TryLock();
    void Unlock();
    bool IsLocked() const { return m_locked.load(); }
    
    // Optimization hints
    void SetUpdatePriority(float priority) { m_updatePriority = priority; }
    float GetUpdatePriority() const { return m_updatePriority; }
    
    // Memory management
    void Compress(); // Compress inactive regions
    void Decompress(); // Prepare for active use
    bool IsCompressed() const { return m_compressed; }

private:
    // Chunk coordinates
    int m_chunkX, m_chunkY;
    int m_worldOffsetX, m_worldOffsetY;
    
    // State
    std::atomic<ChunkState> m_state{ChunkState::Inactive};
    std::atomic<uint32_t> m_sleepTimer{0};
    std::atomic<bool> m_locked{false};
    
    // Active region tracking (for partial updates)
    bool m_hasActiveRegion = false;
    int m_activeMinX = 0, m_activeMinY = 0;
    int m_activeMaxX = 0, m_activeMaxY = 0;
    
    // Neighbor activity (8-directional)
    std::bitset<8> m_neighborActivity;
    
    // Performance tracking
    std::atomic<uint32_t> m_updateCount{0};
    std::atomic<float> m_lastUpdateTime{0.0f};
    std::atomic<uint32_t> m_activeCellCount{0};
    std::atomic<float> m_updatePriority{1.0f};
    
    // Memory optimization
    bool m_compressed = false;
    
    // Constants
    static constexpr uint32_t SLEEP_THRESHOLD = 60; // Frames before considering sleep
    static constexpr float MIN_UPDATE_PRIORITY = 0.1f;
    static constexpr float MAX_UPDATE_PRIORITY = 2.0f;
};

// Chunk coordinate conversion utilities
inline int WorldToChunkCoord(int worldCoord) {
    return worldCoord >= 0 ? worldCoord / CHUNK_SIZE : (worldCoord - CHUNK_SIZE + 1) / CHUNK_SIZE;
}

inline int ChunkToWorldCoord(int chunkCoord) {
    return chunkCoord * CHUNK_SIZE;
}

inline void WorldToChunkLocal(int worldX, int worldY, int& chunkX, int& chunkY, int& localX, int& localY) {
    chunkX = WorldToChunkCoord(worldX);
    chunkY = WorldToChunkCoord(worldY);
    localX = worldX - ChunkToWorldCoord(chunkX);
    localY = worldY - ChunkToWorldCoord(chunkY);
}

// Direction constants for neighbor tracking
enum ChunkDirection : int {
    CHUNK_DIR_NW = 0, CHUNK_DIR_N = 1, CHUNK_DIR_NE = 2,
    CHUNK_DIR_W  = 3,                  CHUNK_DIR_E  = 4,
    CHUNK_DIR_SW = 5, CHUNK_DIR_S = 6, CHUNK_DIR_SE = 7
};

} // namespace BGE