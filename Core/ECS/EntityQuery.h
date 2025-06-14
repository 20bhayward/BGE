#pragma once

#include "../Entity.h"
#include <vector>
#include <functional>
#include <typeindex>

namespace BGE {

// Query builder for finding entities with specific component combinations
class EntityQuery {
public:
    EntityQuery() = default;
    
    // Add required component types
    template<typename T>
    EntityQuery& With() {
        m_requiredComponents.push_back(std::type_index(typeid(T)));
        return *this;
    }
    
    // Add excluded component types
    template<typename T>
    EntityQuery& Without() {
        m_excludedComponents.push_back(std::type_index(typeid(T)));
        return *this;
    }
    
    // Add filter predicate
    EntityQuery& Where(std::function<bool(Entity*)> predicate) {
        m_predicates.push_back(predicate);
        return *this;
    }
    
    // Execute query and return matching entities
    std::vector<Entity*> Execute();
    
    // Execute query with callback (avoids allocation)
    void ForEach(std::function<void(Entity*)> callback);
    
    // Get first matching entity (or nullptr)
    Entity* First();
    
    // Count matching entities
    size_t Count();
    
private:
    std::vector<std::type_index> m_requiredComponents;
    std::vector<std::type_index> m_excludedComponents;
    std::vector<std::function<bool(Entity*)>> m_predicates;
    
    bool MatchesEntity(Entity* entity) const;
};

// Convenience query functions
namespace Query {
    // Get all entities with specific component
    template<typename T>
    std::vector<Entity*> GetEntitiesWith() {
        return EntityQuery().With<T>().Execute();
    }
    
    // Get all entities with multiple components
    template<typename T1, typename T2>
    std::vector<Entity*> GetEntitiesWith() {
        return EntityQuery().With<T1>().With<T2>().Execute();
    }
    
    template<typename T1, typename T2, typename T3>
    std::vector<Entity*> GetEntitiesWith() {
        return EntityQuery().With<T1>().With<T2>().With<T3>().Execute();
    }
}

} // namespace BGE