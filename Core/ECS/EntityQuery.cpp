#include "EntityQuery.h"
#include "../Entity.h"

namespace BGE {

std::vector<Entity*> EntityQuery::Execute() {
    std::vector<Entity*> results;
    
    auto& entityManager = EntityManager::Instance();
    
    // For now, iterate all entities (can be optimized with archetype system)
    // In a full implementation, this would use the archetype system for fast queries
    for (auto& entity : entityManager.GetAllEntities()) {
        if (MatchesEntity(entity.second.get())) {
            results.push_back(entity.second.get());
        }
    }
    
    return results;
}

void EntityQuery::ForEach(std::function<void(Entity*)> callback) {
    auto& entityManager = EntityManager::Instance();
    
    for (auto& entity : entityManager.GetAllEntities()) {
        if (MatchesEntity(entity.second.get())) {
            callback(entity.second.get());
        }
    }
}

Entity* EntityQuery::First() {
    auto& entityManager = EntityManager::Instance();
    
    for (auto& entity : entityManager.GetAllEntities()) {
        if (MatchesEntity(entity.second.get())) {
            return entity.second.get();
        }
    }
    
    return nullptr;
}

size_t EntityQuery::Count() {
    size_t count = 0;
    
    auto& entityManager = EntityManager::Instance();
    
    for (auto& entity : entityManager.GetAllEntities()) {
        if (MatchesEntity(entity.second.get())) {
            count++;
        }
    }
    
    return count;
}

bool EntityQuery::MatchesEntity(Entity* entity) const {
    if (!entity || !entity->IsActive()) {
        return false;
    }
    
    // Check required components
    for (const auto& componentType : m_requiredComponents) {
        if (!entity->HasComponentType(componentType)) {
            return false;
        }
    }
    
    // Check excluded components
    for (const auto& componentType : m_excludedComponents) {
        if (entity->HasComponentType(componentType)) {
            return false;
        }
    }
    
    // Check predicates
    for (const auto& predicate : m_predicates) {
        if (!predicate(entity)) {
            return false;
        }
    }
    
    return true;
}

} // namespace BGE