#pragma once

#include <cstddef>

namespace BGE {

// ECS configuration settings
struct ECSConfig {
    // Memory pool settings
    size_t componentPoolBlockSize = 1024;      // Number of components per pool block
    size_t initialEntityCapacity = 10000;      // Initial entity array capacity
    size_t maxEntities = 1000000;              // Maximum number of entities
    size_t maxComponentTypes = 512;            // Maximum component types
    
    // Performance settings
    bool enableQueryCaching = true;            // Enable query result caching
    bool enableParallelQueries = true;         // Enable parallel query execution
    size_t queryBatchSize = 1000;              // Entities per batch for parallel processing
    
    // Memory settings
    bool enableMemoryPooling = true;           // Use memory pooling for components
    size_t arenaAllocatorSize = 16 * 1024 * 1024; // 16MB for temporary allocations
    
    // Debug settings
    bool enableProfiling = false;              // Enable performance profiling
    bool enableValidation = true;              // Enable validation checks
    bool enableMemoryTracking = false;         // Track memory usage statistics
    
    // Thread settings
    size_t workerThreadCount = 0;              // 0 = auto-detect based on CPU cores
    bool enableThreadSafety = true;            // Enable thread-safe operations
    
    // Get singleton instance
    static ECSConfig& Instance() {
        static ECSConfig instance;
        return instance;
    }
    
    // Apply configuration (must be called before ECS initialization)
    void Apply() {
        // This would be called early in engine initialization
        // to configure the ECS before first use
    }
    
private:
    ECSConfig() = default;
};

// Helper to configure ECS
class ECSConfigurator {
public:
    static void SetComponentPoolBlockSize(size_t size) {
        ECSConfig::Instance().componentPoolBlockSize = size;
    }
    
    static void SetMaxEntities(size_t count) {
        ECSConfig::Instance().maxEntities = count;
    }
    
    static void SetMaxComponentTypes(size_t count) {
        ECSConfig::Instance().maxComponentTypes = count;
    }
    
    static void EnableMemoryPooling(bool enable) {
        ECSConfig::Instance().enableMemoryPooling = enable;
    }
    
    static void EnableQueryCaching(bool enable) {
        ECSConfig::Instance().enableQueryCaching = enable;
    }
    
    static void EnableParallelQueries(bool enable) {
        ECSConfig::Instance().enableParallelQueries = enable;
    }
    
    static void SetWorkerThreadCount(size_t count) {
        ECSConfig::Instance().workerThreadCount = count;
    }
    
    static void EnableProfiling(bool enable) {
        ECSConfig::Instance().enableProfiling = enable;
    }
    
    static void EnableValidation(bool enable) {
        ECSConfig::Instance().enableValidation = enable;
    }
    
    static void EnableMemoryTracking(bool enable) {
        ECSConfig::Instance().enableMemoryTracking = enable;
    }
};

} // namespace BGE