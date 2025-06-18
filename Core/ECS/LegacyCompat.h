#pragma once

#include "EntityManager.h"
#include "EntityQuery.h"
#include "../Entity.h"

namespace BGE {

// Legacy compatibility wrapper for existing code
class LegacyEntityManager {
public:
    static LegacyEntityManager& Instance() {
        static LegacyEntityManager instance;
        return instance;
    }
    
    // Create a legacy Entity* wrapper
    Entity* CreateEntity(const std::string& name = "") {
        EntityID id = EntityManager::Instance().CreateEntity(name);
        auto entity = std::make_unique<Entity>(id.id, name);
        Entity* ptr = entity.get();
        m_entities[id.id] = std::move(entity);
        return ptr;
    }
    
    void DestroyEntity(EntityID id) {
        EntityManager::Instance().DestroyEntity(EntityID(id));
        m_entities.erase(id);
    }
    
    void DestroyEntity(Entity* entity) {
        if (entity) {
            DestroyEntity(entity->GetID());
        }
    }
    
    Entity* GetEntity(EntityID id) {
        auto it = m_entities.find(id);
        if (it != m_entities.end()) {
            return it->second.get();
        }
        
        // Try to create wrapper for existing entity
        EntityID entityId(id);
        if (EntityManager::Instance().IsEntityValid(entityId)) {
            std::string name = EntityManager::Instance().GetEntityName(entityId);
            auto entity = std::make_unique<Entity>(id, name);
            Entity* ptr = entity.get();
            m_entities[id] = std::move(entity);
            return ptr;
        }
        
        return nullptr;
    }
    
    const Entity* GetEntity(EntityID id) const {
        auto it = m_entities.find(id);
        return it != m_entities.end() ? it->second.get() : nullptr;
    }
    
    template<typename T>
    std::vector<Entity*> GetEntitiesWithComponent() {
        std::vector<Entity*> result;
        
        auto& entityManager = EntityManager::Instance();
        EntityQuery query(&entityManager);
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
        m_entities.clear();
    }
    
    size_t GetEntityCount() const {
        return EntityManager::Instance().GetEntityCount();
    }
    
    // Mock GetAllEntities for compatibility
    struct EntityCollection {
        std::unordered_map<uint64_t, std::unique_ptr<Entity>>& entities;
        EntityCollection(std::unordered_map<uint64_t, std::unique_ptr<Entity>>& e) : entities(e) {}
        
        auto begin() { return entities.begin(); }
        auto end() { return entities.end(); }
        size_t size() const { return entities.size(); }
    };
    
    EntityCollection GetAllEntities() {
        // Refresh entity list from ECS
        RefreshEntityCache();
        return EntityCollection(m_entities);
    }
    
private:
    std::unordered_map<uint64_t, std::unique_ptr<Entity>> m_entities;
    
    void RefreshEntityCache() {
        // Get all entities from new ECS and create wrappers
        auto& entityManager = EntityManager::Instance();
        auto& archetypeManager = entityManager.GetArchetypeManager();
        
        // Clear old cache
        m_entities.clear();
        
        // Iterate through all archetypes to find entities
        for (const auto& archetype : archetypeManager.GetAllArchetypes()) {
            if (!archetype) continue;
            
            const auto& entities = archetype->GetEntities();
            for (EntityID id : entities) {
                if (!m_entities.count(id.id)) {
                    std::string name = entityManager.GetEntityName(id);
                    m_entities[id.id] = std::make_unique<Entity>(id.id, name);
                }
            }
        }
    }
};

} // namespace BGE

// Redirect EntityManager to use legacy compatibility
#ifdef EntityManager
#undef EntityManager
#endif
#define EntityManager LegacyEntityManager