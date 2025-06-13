#include "ChunkManager.h"
#include "../SimulationWorld.h"
#include "../../Core/Threading/ThreadPool.h"

namespace BGE {

ChunkManager::ChunkManager(SimulationWorld* world) : m_world(world) {
}

ChunkManager::~ChunkManager() = default;

void ChunkManager::Update(float deltaTime) {
    if (m_threadPool && m_maxConcurrentChunks > 1) {
        UpdateParallel(deltaTime);
    } else {
        // Sequential update
        std::vector<Chunk*> chunksToUpdate;
        
        {
            std::shared_lock lock(m_chunksMutex);
            for (auto& pair : m_chunks) {
                if (pair.second->ShouldUpdate()) {
                    chunksToUpdate.push_back(pair.second.get());
                }
            }
        }
        
        for (Chunk* chunk : chunksToUpdate) {
            UpdateChunk(chunk, deltaTime);
        }
    }
    
    // Cleanup inactive chunks periodically
    static float cleanupTimer = 0.0f;
    cleanupTimer += deltaTime;
    if (cleanupTimer >= m_chunkUnloadDelay) {
        UnloadInactiveChunks();
        cleanupTimer = 0.0f;
    }
}

void ChunkManager::UpdateParallel(float deltaTime) {
    // TODO: Implement parallel chunk updates using thread pool
    Update(deltaTime); // Fallback to sequential for now
}

Chunk* ChunkManager::GetChunk(int chunkX, int chunkY) {
    ChunkCoord coord{chunkX, chunkY};
    
    std::shared_lock lock(m_chunksMutex);
    auto it = m_chunks.find(coord);
    return (it != m_chunks.end()) ? it->second.get() : nullptr;
}

Chunk* ChunkManager::GetOrCreateChunk(int chunkX, int chunkY) {
    ChunkCoord coord{chunkX, chunkY};
    
    {
        std::shared_lock lock(m_chunksMutex);
        auto it = m_chunks.find(coord);
        if (it != m_chunks.end()) {
            return it->second.get();
        }
    }
    
    // Create new chunk
    std::unique_lock lock(m_chunksMutex);
    
    // Double-check in case another thread created it
    auto it = m_chunks.find(coord);
    if (it != m_chunks.end()) {
        return it->second.get();
    }
    
    auto chunk = CreateChunk(chunkX, chunkY);
    Chunk* chunkPtr = chunk.get();
    m_chunks[coord] = std::move(chunk);
    
    return chunkPtr;
}

const Chunk* ChunkManager::GetChunk(int chunkX, int chunkY) const {
    ChunkCoord coord{chunkX, chunkY};
    
    std::shared_lock lock(m_chunksMutex);
    auto it = m_chunks.find(coord);
    return (it != m_chunks.end()) ? it->second.get() : nullptr;
}

void ChunkManager::ActivateChunk(int chunkX, int chunkY) {
    if (Chunk* chunk = GetOrCreateChunk(chunkX, chunkY)) {
        chunk->SetState(ChunkState::Active);
        PropagateActivity(chunkX, chunkY);
    }
}

void ChunkManager::DeactivateChunk(int chunkX, int chunkY) {
    if (Chunk* chunk = GetChunk(chunkX, chunkY)) {
        chunk->SetState(ChunkState::Inactive);
    }
}

void ChunkManager::UnloadChunk(int chunkX, int chunkY) {
    ChunkCoord coord{chunkX, chunkY};
    
    std::unique_lock lock(m_chunksMutex);
    m_chunks.erase(coord);
}

void ChunkManager::UnloadInactiveChunks() {
    std::vector<ChunkCoord> chunksToUnload;
    
    {
        std::shared_lock lock(m_chunksMutex);
        for (const auto& pair : m_chunks) {
            if (pair.second->GetState() == ChunkState::Inactive) {
                chunksToUnload.push_back(pair.first);
            }
        }
    }
    
    for (const ChunkCoord& coord : chunksToUnload) {
        UnloadChunk(coord.x, coord.y);
    }
}

Chunk* ChunkManager::GetChunkForWorldPos(int worldX, int worldY) {
    int chunkX = WorldToChunkCoord(worldX);
    int chunkY = WorldToChunkCoord(worldY);
    return GetChunk(chunkX, chunkY);
}

void ChunkManager::GetChunksInRegion(int x1, int y1, int x2, int y2, std::vector<Chunk*>& chunks) {
    int chunkX1 = WorldToChunkCoord(x1);
    int chunkY1 = WorldToChunkCoord(y1);
    int chunkX2 = WorldToChunkCoord(x2);
    int chunkY2 = WorldToChunkCoord(y2);
    
    chunks.clear();
    
    for (int cy = chunkY1; cy <= chunkY2; ++cy) {
        for (int cx = chunkX1; cx <= chunkX2; ++cx) {
            if (Chunk* chunk = GetChunk(cx, cy)) {
                chunks.push_back(chunk);
            }
        }
    }
}

void ChunkManager::MarkRegionActive(int x1, int y1, int x2, int y2) {
    std::vector<Chunk*> chunks;
    GetChunksInRegion(x1, y1, x2, y2, chunks);
    
    for (Chunk* chunk : chunks) {
        chunk->MarkActive();
    }
}

void ChunkManager::MarkChunkDirty(int chunkX, int chunkY) {
    if (Chunk* chunk = GetChunk(chunkX, chunkY)) {
        chunk->MarkDirty();
    }
}

void ChunkManager::PropagateActivity(int chunkX, int chunkY) {
    // Activate neighboring chunks
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            
            int neighborX = chunkX + dx;
            int neighborY = chunkY + dy;
            
            if (Chunk* neighbor = GetChunk(neighborX, neighborY)) {
                neighbor->MarkActive();
            }
        }
    }
}

uint32_t ChunkManager::GetActiveChunkCount() const {
    uint32_t count = 0;
    
    std::shared_lock lock(m_chunksMutex);
    for (const auto& pair : m_chunks) {
        if (pair.second->IsActive()) {
            ++count;
        }
    }
    
    return count;
}

uint32_t ChunkManager::GetDirtyChunkCount() const {
    uint32_t count = 0;
    
    std::shared_lock lock(m_chunksMutex);
    for (const auto& pair : m_chunks) {
        if (pair.second->IsDirty()) {
            ++count;
        }
    }
    
    return count;
}

std::unique_ptr<Chunk> ChunkManager::CreateChunk(int chunkX, int chunkY) {
    int worldOffsetX = ChunkToWorldCoord(chunkX);
    int worldOffsetY = ChunkToWorldCoord(chunkY);
    
    return std::make_unique<Chunk>(chunkX, chunkY, worldOffsetX, worldOffsetY);
}

void ChunkManager::UpdateChunk(Chunk* chunk, float deltaTime) {
    if (chunk && chunk->TryLock()) {
        chunk->Update(deltaTime);
        chunk->Unlock();
    }
}

std::vector<ChunkCoord> ChunkManager::GetNeighborCoords(int chunkX, int chunkY) const {
    std::vector<ChunkCoord> neighbors;
    neighbors.reserve(8);
    
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            neighbors.push_back({chunkX + dx, chunkY + dy});
        }
    }
    
    return neighbors;
}

void ChunkManager::CompressInactiveChunks() {
    std::shared_lock lock(m_chunksMutex);
    for (auto& pair : m_chunks) {
        if (pair.second->GetState() == ChunkState::Inactive) {
            pair.second->Compress();
        }
    }
}

size_t ChunkManager::GetMemoryUsage() const {
    // TODO: Calculate actual memory usage
    return m_chunks.size() * sizeof(Chunk);
}

void ChunkManager::GetChunkBounds(int& minX, int& minY, int& maxX, int& maxY) const {
    if (m_chunks.empty()) {
        minX = minY = maxX = maxY = 0;
        return;
    }
    
    std::shared_lock lock(m_chunksMutex);
    auto it = m_chunks.begin();
    minX = maxX = it->first.x;
    minY = maxY = it->first.y;
    
    for (const auto& pair : m_chunks) {
        minX = std::min(minX, pair.first.x);
        maxX = std::max(maxX, pair.first.x);
        minY = std::min(minY, pair.first.y);
        maxY = std::max(maxY, pair.first.y);
    }
}

std::vector<ChunkCoord> ChunkManager::GetActiveChunkCoords() const {
    std::vector<ChunkCoord> activeChunks;
    
    std::shared_lock lock(m_chunksMutex);
    for (const auto& pair : m_chunks) {
        if (pair.second->IsActive()) {
            activeChunks.push_back(pair.first);
        }
    }
    
    return activeChunks;
}

void ChunkManager::DumpChunkStats() const {
    // TODO: Implement debug output
}

} // namespace BGE