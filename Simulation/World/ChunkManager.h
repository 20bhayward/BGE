#pragma once

#include "Chunk.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <functional>

namespace BGE {

class SimulationWorld;
class ThreadPool;

struct ChunkCoord {
    int x, y;
    
    bool operator==(const ChunkCoord& other) const {
        return x == other.x && y == other.y;
    }
    
    struct Hash {
        size_t operator()(const ChunkCoord& coord) const {
            return std::hash<int>()(coord.x) ^ (std::hash<int>()(coord.y) << 1);
        }
    };
};

class ChunkManager {
public:
    explicit ChunkManager(SimulationWorld* world);
    ~ChunkManager();
    
    // Core update methods
    void Update(float deltaTime);
    void UpdateParallel(float deltaTime);
    
    // Chunk access
    Chunk* GetChunk(int chunkX, int chunkY);
    Chunk* GetOrCreateChunk(int chunkX, int chunkY);
    const Chunk* GetChunk(int chunkX, int chunkY) const;
    
    // Chunk lifecycle
    void ActivateChunk(int chunkX, int chunkY);
    void DeactivateChunk(int chunkX, int chunkY);
    void UnloadChunk(int chunkX, int chunkY);
    void UnloadInactiveChunks();
    
    // World position to chunk mapping
    Chunk* GetChunkForWorldPos(int worldX, int worldY);
    void GetChunksInRegion(int x1, int y1, int x2, int y2, std::vector<Chunk*>& chunks);
    
    // Activity management
    void MarkRegionActive(int x1, int y1, int x2, int y2);
    void MarkChunkDirty(int chunkX, int chunkY);
    void PropagateActivity(int chunkX, int chunkY);
    
    // Update scheduling
    void ScheduleChunkUpdate(Chunk* chunk, float priority = 1.0f);
    void ProcessScheduledUpdates(float deltaTime);
    
    // Performance optimization
    void SetMaxActiveChunks(uint32_t maxChunks) { m_maxActiveChunks = maxChunks; }
    void SetChunkUnloadDelay(float delay) { m_chunkUnloadDelay = delay; }
    void EnableAdaptiveUpdates(bool enable) { m_adaptiveUpdates = enable; }
    
    // Statistics
    uint32_t GetLoadedChunkCount() const { return static_cast<uint32_t>(m_chunks.size()); }
    uint32_t GetActiveChunkCount() const;
    uint32_t GetDirtyChunkCount() const;
    
    // Thread management
    void SetThreadPool(ThreadPool* threadPool) { m_threadPool = threadPool; }
    void SetMaxConcurrentChunks(uint32_t maxConcurrent) { m_maxConcurrentChunks = maxConcurrent; }
    
    // Memory management
    void CompressInactiveChunks();
    size_t GetMemoryUsage() const;
    void SetMemoryLimit(size_t limitBytes) { m_memoryLimit = limitBytes; }
    
    // Debug utilities
    void GetChunkBounds(int& minX, int& minY, int& maxX, int& maxY) const;
    std::vector<ChunkCoord> GetActiveChunkCoords() const;
    void DumpChunkStats() const;

private:
    // Internal update methods
    void UpdateChunk(Chunk* chunk, float deltaTime);
    void UpdateChunkSerial(Chunk* chunk, float deltaTime);
    void UpdateChunkParallel(Chunk* chunk, float deltaTime);
    
    // Chunk creation and cleanup
    std::unique_ptr<Chunk> CreateChunk(int chunkX, int chunkY);
    void CleanupChunk(Chunk* chunk);
    
    // Activity propagation
    void UpdateNeighborActivity(int chunkX, int chunkY);
    std::vector<ChunkCoord> GetNeighborCoords(int chunkX, int chunkY) const;
    
    // Load balancing
    void BalanceChunkLoad();
    float CalculateChunkPriority(const Chunk* chunk) const;
    
    // Memory management
    void EnforceMemoryLimit();
    std::vector<Chunk*> GetChunksForUnload();
    
    SimulationWorld* m_world;
    ThreadPool* m_threadPool = nullptr;
    
    // Chunk storage
    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoord::Hash> m_chunks;
    mutable std::shared_mutex m_chunksMutex;
    
    // Update scheduling
    std::vector<std::pair<Chunk*, float>> m_updateQueue; // chunk, priority
    std::mutex m_updateQueueMutex;
    
    // Active chunk tracking
    std::vector<Chunk*> m_activeChunks;
    std::vector<Chunk*> m_dirtyChunks;
    
    // Settings
    uint32_t m_maxActiveChunks = 1000;
    uint32_t m_maxConcurrentChunks = 8;
    float m_chunkUnloadDelay = 5.0f; // seconds
    bool m_adaptiveUpdates = true;
    
    // Memory management
    size_t m_memoryLimit = 1024 * 1024 * 1024; // 1GB default
    
    // Performance tracking
    mutable std::mutex m_statsMutex;
    uint64_t m_totalUpdates = 0;
    float m_averageUpdateTime = 0.0f;
    
    // Update patterns for optimization
    enum class UpdatePattern {
        Sequential,    // Update chunks in order
        Checkerboard,  // Alternate updates for parallelism
        PriorityBased, // Update high-priority chunks first
        Adaptive       // Dynamically choose pattern
    };
    
    UpdatePattern m_updatePattern = UpdatePattern::Adaptive;
    
    // Constants
    static constexpr float NEIGHBOR_ACTIVATION_THRESHOLD = 0.1f;
    static constexpr uint32_t MAX_CHUNKS_PER_FRAME = 16;
    static constexpr float CHUNK_PRIORITY_DECAY = 0.95f;
};

} // namespace BGE