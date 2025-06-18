#pragma once

#include "../Entity.h"
#include "EntityManager.h"
#include "Components/CoreComponents.h"
#include "Components/GameplayComponents.h"

namespace BGE {

// This wrapper provides backward compatibility for the existing Entity class
// It redirects all operations to the new ECS system
class LegacyEntityWrapper {
public:
    static Entity* CreateCompatibleEntity(EntityID id, const std::string& name) {
        // Create a legacy Entity object that wraps the new ECS entity
        return new Entity(id.id, name);
    }
    
    template<typename T>
    static T* AddComponentToEntity(Entity* entity, T&& component) {
        if (!entity) return nullptr;
        
        EntityID id(entity->GetID(), 0); // Assume generation 0 for legacy entities
        return EntityManager::Instance().AddComponent<T>(id, std::forward<T>(component));
    }
    
    template<typename T>
    static T* GetComponentFromEntity(Entity* entity) {
        if (!entity) return nullptr;
        
        EntityID id(entity->GetID(), 0);
        return EntityManager::Instance().GetComponent<T>(id);
    }
    
    template<typename T>
    static bool HasComponentOnEntity(Entity* entity) {
        if (!entity) return false;
        
        EntityID id(entity->GetID(), 0);
        return EntityManager::Instance().HasComponent<T>(id);
    }
    
    template<typename T>
    static void RemoveComponentFromEntity(Entity* entity) {
        if (!entity) return;
        
        EntityID id(entity->GetID(), 0);
        EntityManager::Instance().RemoveComponent<T>(id);
    }
};

// Update the old EntityManager to use the new system
class LegacyEntityManagerAdapter {
public:
    static EntityManager& Instance() {
        return EntityManager::Instance();
    }
    
    Entity* CreateEntity(const std::string& name = "") {
        EntityID newId = EntityManager::Instance().CreateEntity(name);
        
        // Create a legacy entity wrapper
        Entity* legacyEntity = new Entity(newId.id, name);
        m_legacyEntities[newId.id] = std::unique_ptr<Entity>(legacyEntity);
        
        return legacyEntity;
    }
    
    void DestroyEntity(EntityID id) {
        EntityManager::Instance().DestroyEntity(EntityID(id, 0));
        m_legacyEntities.erase(id);
    }
    
    void DestroyEntity(Entity* entity) {
        if (entity) {
            DestroyEntity(entity->GetID());
        }
    }
    
    Entity* GetEntity(EntityID id) {
        auto it = m_legacyEntities.find(id);
        return it != m_legacyEntities.end() ? it->second.get() : nullptr;
    }
    
    template<typename T>
    std::vector<Entity*> GetEntitiesWithComponent() {
        std::vector<Entity*> result;
        
        EntityQuery query(&EntityManager::Instance());
        query.With<T>().ForEach([&](EntityID id) {
            Entity* entity = GetEntity(id.id);
            if (entity) {
                result.push_back(entity);
            }
        });
        
        return result;
    }
    
    void Clear() {
        EntityManager::Instance().Clear();
        m_legacyEntities.clear();
    }
    
    size_t GetEntityCount() const {
        return EntityManager::Instance().GetEntityCount();
    }
    
private:
    std::unordered_map<uint64_t, std::unique_ptr<Entity>> m_legacyEntities;
};

} // namespace BGE