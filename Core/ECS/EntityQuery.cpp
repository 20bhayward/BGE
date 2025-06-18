#include "EntityQuery.h"
#include "../Entity.h"  // Need full definition before EntityManager.h
#include "EntityManager.h"

namespace BGE {

EntityQuery::EntityQuery(EntityManager* manager)
    : m_entityManager(manager) {
}

QueryResult EntityQuery::Execute() {
    auto& archetypeManager = m_entityManager->GetArchetypeManager();
    std::vector<uint32_t> matchingArchetypes = archetypeManager.GetArchetypesMatching(m_requiredMask, m_excludedMask);
    
    // Filter archetypes based on component filters if any
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
    
    return QueryResult(matchingArchetypes, &archetypeManager);
}

void EntityQuery::ForEach(std::function<void(EntityID)> callback) {
    QueryResult result = Execute();
    
    for (auto entityData : result) {
        callback(entityData.entity);
    }
}

EntityID EntityQuery::First() {
    QueryResult result = Execute();
    
    for (auto entityData : result) {
        return entityData.entity;
    }
    
    return INVALID_ENTITY;
}

size_t EntityQuery::Count() {
    QueryResult result = Execute();
    return result.Count();
}

bool EntityQuery::PassesFilters(Archetype* archetype, uint32_t row) const {
    for (const auto& [typeID, filter] : m_filters) {
        IComponentStorage* storage = archetype->GetComponentStorage(typeID);
        if (!storage) return false;
        
        const void* component = storage->GetRaw(row);
        if (!component || !filter(component)) {
            return false;
        }
    }
    
    return true;
}

} // namespace BGE