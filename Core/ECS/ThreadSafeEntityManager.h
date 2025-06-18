#pragma once

#include "EntityManager.h"
#include <shared_mutex>
#include <atomic>

namespace BGE {

// Thread-safe wrapper for EntityManager
// This can be used as a drop-in replacement with proper synchronization
class ThreadSafeEntityManager {
public:
    static ThreadSafeEntityManager& Instance() {
        static ThreadSafeEntityManager instance;
        return instance;
    }
    
    // Entity creation - requires exclusive lock
    EntityID CreateEntity(const std::string& name = "") {
        std::unique_lock lock(m_mutex);
        return m_manager.CreateEntity(name);
    }
    
    // Entity destruction - requires exclusive lock
    void DestroyEntity(EntityID entity) {
        std::unique_lock lock(m_mutex);
        m_manager.DestroyEntity(entity);
    }
    
    // Entity validation - can use shared lock
    bool IsEntityValid(EntityID entity) const {
        std::shared_lock lock(m_mutex);
        return m_manager.IsEntityValid(entity);
    }
    
    // Component operations - require exclusive lock for modifications
    template<typename T>
    T* AddComponent(EntityID entity, T&& component) {
        std::unique_lock lock(m_mutex);
        return m_manager.AddComponent(entity, std::forward<T>(component));
    }
    
    template<typename T>
    void RemoveComponent(EntityID entity) {
        std::unique_lock lock(m_mutex);
        m_manager.RemoveComponent<T>(entity);
    }
    
    // Component queries - can use shared lock
    template<typename T>
    T* GetComponent(EntityID entity) {
        std::shared_lock lock(m_mutex);
        return m_manager.GetComponent<T>(entity);
    }
    
    template<typename T>
    bool HasComponent(EntityID entity) const {
        std::shared_lock lock(m_mutex);
        return m_manager.HasComponent<T>(entity);
    }
    
    // Entity info - shared locks
    const std::string& GetEntityName(EntityID entity) const {
        std::shared_lock lock(m_mutex);
        return m_manager.GetEntityName(entity);
    }
    
    void SetEntityName(EntityID entity, const std::string& name) {
        std::unique_lock lock(m_mutex);
        m_manager.SetEntityName(entity, name);
    }
    
    size_t GetEntityCount() const {
        std::shared_lock lock(m_mutex);
        return m_manager.GetEntityCount();
    }
    
    // Query support - needs careful handling
    class ThreadSafeQuery {
    public:
        ThreadSafeQuery(ThreadSafeEntityManager* manager) : m_manager(manager) {}
        
        template<typename Func>
        void ForEach(Func&& func) {
            // Take a snapshot of matching entities under lock
            std::vector<EntityID> entities;
            {
                std::shared_lock lock(m_manager->m_mutex);
                // Collect entities that match query criteria
                // This is simplified - real implementation would use EntityQuery
            }
            
            // Process entities without holding lock
            // This allows parallel processing but entities might be invalid
            for (EntityID entity : entities) {
                if (m_manager->IsEntityValid(entity)) {
                    func(entity);
                }
            }
        }
        
    private:
        ThreadSafeEntityManager* m_manager;
    };
    
    // Clear all entities - exclusive lock
    void Clear() {
        std::unique_lock lock(m_mutex);
        m_manager.Clear();
    }
    
    // Get entity generation for debugging
    uint32_t GetEntityGeneration(EntityID entity) const {
        std::shared_lock lock(m_mutex);
        return m_manager.GetEntityGeneration(entity);
    }
    
    // Performance monitoring
    struct Stats {
        std::atomic<uint64_t> createCount{0};
        std::atomic<uint64_t> destroyCount{0};
        std::atomic<uint64_t> componentAddCount{0};
        std::atomic<uint64_t> componentRemoveCount{0};
        std::atomic<uint64_t> lockContentions{0};
    };
    
    const Stats& GetStats() const { return m_stats; }
    
private:
    ThreadSafeEntityManager() = default;
    
    mutable std::shared_mutex m_mutex;
    EntityManager m_manager;
    mutable Stats m_stats;
};

// Alternative: Lock-free entity manager using atomic operations
class LockFreeEntityManager {
public:
    // This would use atomic operations and lock-free data structures
    // Implementation would be complex but provide better scalability
    // Key techniques:
    // - Atomic entity ID generation
    // - Lock-free queue for free entity indices
    // - Hazard pointers for safe memory reclamation
    // - Copy-on-write for archetype transitions
};

} // namespace BGE