#pragma once

#include "EntityManager.h"
#include "EntityQuery.h"
#include "../Entity.h"
#include <unordered_map>
#include <memory>

namespace BGE {

// Compatibility wrapper to make old code work with new ECS
class EntityManagerCompat {
public:
    static EntityManagerCompat& Instance() {
        static EntityManagerCompat instance;
        return instance;
    }
    
    // Legacy API that returns Entity* objects
    std::unordered_map<uint64_t, std::unique_ptr<Entity>> GetAllEntities() {
        std::unordered_map<uint64_t, std::unique_ptr<Entity>> result;
        
        auto& entityManager = EntityManager::Instance();
        auto& archetypeManager = entityManager.GetArchetypeManager();
        
        // Iterate through all archetypes
        for (const auto& archetype : archetypeManager.GetAllArchetypes()) {
            if (!archetype) continue;
            
            const auto& entities = archetype->GetEntities();
            for (EntityID id : entities) {
                if (!result.count(id.id)) {
                    // Create a legacy Entity wrapper
                    auto entity = std::make_unique<Entity>(id.id, entityManager.GetEntityName(id));
                    result[id.id] = std::move(entity);
                }
            }
        }
        
        return result;
    }
    
    // For iteration in range-based for loops
    class EntityIterator {
    public:
        EntityIterator(EntityManager* manager) : m_entityManager(manager) {
            m_query = std::make_unique<EntityQuery>(manager);
        }
        
        struct EntityProxy {
            EntityID id;
            EntityManager* manager;
            
            Entity* operator->() {
                static thread_local std::unordered_map<uint64_t, std::unique_ptr<Entity>> cache;
                if (!cache.count(id.id)) {
                    cache[id.id] = std::make_unique<Entity>(id.id, manager->GetEntityName(id));
                }
                return cache[id.id].get();
            }
            
            template<typename T>
            T* GetComponent() {
                return manager->GetComponent<T>(id);
            }
        };
        
        class Iterator {
        public:
            Iterator(QueryResult::Iterator it, EntityManager* mgr) 
                : m_iter(it), m_manager(mgr) {}
            
            Iterator& operator++() { 
                ++m_iter; 
                return *this; 
            }
            
            bool operator!=(const Iterator& other) const { 
                return m_iter != other.m_iter; 
            }
            
            std::pair<uint64_t, EntityProxy> operator*() {
                auto data = *m_iter;
                return { data.entity.id, EntityProxy{data.entity, m_manager} };
            }
            
        private:
            QueryResult::Iterator m_iter;
            EntityManager* m_manager;
        };
        
        Iterator begin() { 
            auto result = m_query->Execute();
            return Iterator(result.begin(), m_entityManager); 
        }
        
        Iterator end() { 
            auto result = m_query->Execute();
            return Iterator(result.end(), m_entityManager); 
        }
        
    private:
        EntityManager* m_entityManager;
        std::unique_ptr<EntityQuery> m_query;
    };
    
    EntityIterator GetAllEntitiesIterator() {
        return EntityIterator(&EntityManager::Instance());
    }
};

// Helper macros for easier migration
#define ENTITY_MANAGER_COMPAT EntityManagerCompat::Instance()

} // namespace BGE