#include "Chunk.h"

namespace BGE {

Chunk::Chunk(int chunkX, int chunkY, int worldOffsetX, int worldOffsetY)
    : m_chunkX(chunkX), m_chunkY(chunkY)
    , m_worldOffsetX(worldOffsetX), m_worldOffsetY(worldOffsetY) {
}

void Chunk::Update(float deltaTime) {
    if (m_state == ChunkState::Inactive) return;
    
    ++m_updateCount;
    m_lastUpdateTime = deltaTime;
    
    // Update active cell count and other metrics
    // TODO: Implement actual chunk update logic
}

bool Chunk::ShouldUpdate() const {
    return m_state == ChunkState::Active || m_state == ChunkState::Dirty;
}

void Chunk::SetActiveRegion(int minX, int minY, int maxX, int maxY) {
    m_activeMinX = minX;
    m_activeMinY = minY;
    m_activeMaxX = maxX;
    m_activeMaxY = maxY;
    m_hasActiveRegion = true;
}

void Chunk::GetActiveRegion(int& minX, int& minY, int& maxX, int& maxY) const {
    minX = m_activeMinX;
    minY = m_activeMinY;
    maxX = m_activeMaxX;
    maxY = m_activeMaxY;
}

void Chunk::SetNeighborActivity(int direction, bool active) {
    if (direction >= 0 && direction < 8) {
        m_neighborActivity[direction] = active;
    }
}

bool Chunk::GetNeighborActivity(int direction) const {
    if (direction >= 0 && direction < 8) {
        return m_neighborActivity[direction];
    }
    return false;
}

bool Chunk::TryLock() {
    bool expected = false;
    return m_locked.compare_exchange_weak(expected, true);
}

void Chunk::Unlock() {
    m_locked.store(false);
}

void Chunk::Compress() {
    if (m_state == ChunkState::Inactive) {
        m_compressed = true;
        // TODO: Implement compression logic
    }
}

void Chunk::Decompress() {
    if (m_compressed) {
        m_compressed = false;
        // TODO: Implement decompression logic
    }
}

} // namespace BGE