#pragma once

#include "EntityQuery.h"
#include <chrono>
#include <atomic>

namespace BGE {

// Query cache entry
struct QueryCacheEntry {
    std::vector<uint32_t> archetypeIndices;
    std::chrono::steady_clock::time_point lastUpdate;
    uint64_t queryVersion;
    bool valid;
    
    QueryCacheEntry() : queryVersion(0), valid(false) {}
};

// Global query cache version for invalidation
class QueryCacheManager {
public:
    static QueryCacheManager& Instance() {
        static QueryCacheManager instance;
        return instance;
    }
    
    void InvalidateAll() {
        m_globalVersion.fetch_add(1, std::memory_order_release);
    }
    
    uint64_t GetVersion() const {
        return m_globalVersion.load(std::memory_order_acquire);
    }
    
private:
    std::atomic<uint64_t> m_globalVersion{0};
};

// Cached entity query for improved performance
class CachedEntityQuery : public EntityQuery {
public:
    explicit CachedEntityQuery(EntityManager* manager) 
        : EntityQuery(manager), m_cacheEnabled(true) {}
    
    // Override Execute to use caching
    QueryResult Execute() override {
        if (!m_cacheEnabled) {
            return EntityQuery::Execute();
        }
        
        // Check if cache is valid
        uint64_t currentVersion = QueryCacheManager::Instance().GetVersion();
        if (m_cache.valid && m_cache.queryVersion == currentVersion) {
            // Return cached result
            return QueryResult(m_cache.archetypeIndices, &m_entityManager->GetArchetypeManager());
        }
        
        // Execute query and cache result
        auto& archetypeManager = m_entityManager->GetArchetypeManager();
        std::vector<uint32_t> matchingArchetypes = archetypeManager.GetArchetypesMatching(m_requiredMask, m_excludedMask);
        
        // Apply filters if any
        if (!m_filters.empty()) {
            std::vector<uint32_t> filtered;
            
            for (uint32_t archetypeIdx : matchingArchetypes) {
                Archetype* archetype = archetypeManager.GetArchetype(archetypeIdx);
                if (archetype && archetype->GetEntityCount() > 0) {
                    // Check if at least one entity passes filters
                    bool hasMatch = false;
                    for (size_t i = 0; i < archetype->GetEntityCount() && !hasMatch; ++i) {
                        if (PassesFilters(archetype, static_cast<uint32_t>(i))) {
                            hasMatch = true;
                        }
                    }
                    
                    if (hasMatch) {
                        filtered.push_back(archetypeIdx);
                    }
                }
            }
            
            matchingArchetypes = std::move(filtered);
        }
        
        // Update cache
        m_cache.archetypeIndices = matchingArchetypes;
        m_cache.lastUpdate = std::chrono::steady_clock::now();
        m_cache.queryVersion = currentVersion;
        m_cache.valid = true;
        
        return QueryResult(matchingArchetypes, &archetypeManager);
    }
    
    // Control caching
    void SetCacheEnabled(bool enabled) { m_cacheEnabled = enabled; }
    bool IsCacheEnabled() const { return m_cacheEnabled; }
    
    // Force cache invalidation
    void InvalidateCache() {
        m_cache.valid = false;
    }
    
    // Get cache statistics
    bool IsCacheValid() const { return m_cache.valid; }
    
    std::chrono::milliseconds GetCacheAge() const {
        if (!m_cache.valid) return std::chrono::milliseconds(0);
        
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - m_cache.lastUpdate);
    }
    
private:
    QueryCacheEntry m_cache;
    bool m_cacheEnabled;
};

// Query builder with caching support
class CachedQueryBuilder {
public:
    CachedQueryBuilder(EntityManager* manager) 
        : m_query(std::make_unique<CachedEntityQuery>(manager)) {}
    
    template<typename T>
    CachedQueryBuilder& With() {
        m_query->With<T>();
        return *this;
    }
    
    template<typename T>
    CachedQueryBuilder& Without() {
        m_query->Without<T>();
        return *this;
    }
    
    template<typename T>
    CachedQueryBuilder& Where(std::function<bool(const T&)> predicate) {
        m_query->Where<T>(predicate);
        return *this;
    }
    
    CachedQueryBuilder& EnableCache(bool enable = true) {
        m_query->SetCacheEnabled(enable);
        return *this;
    }
    
    std::unique_ptr<CachedEntityQuery> Build() {
        return std::move(m_query);
    }
    
private:
    std::unique_ptr<CachedEntityQuery> m_query;
};

} // namespace BGE