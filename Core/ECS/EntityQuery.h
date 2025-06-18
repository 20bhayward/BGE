#pragma once

#include "EntityID.h"
#include "ComponentRegistry.h"
#include "ArchetypeManager.h"
#include "EntityManager.h"
#include <vector>
#include <functional>
#include <memory>
#include <tuple>

namespace BGE {

// Query filter for component data
template<typename T>
using ComponentFilter = std::function<bool(const T&)>;

// Query result that provides efficient iteration
class QueryResult {
public:
    struct EntityData {
        EntityID entity;
        uint32_t archetypeIndex;
        uint32_t row;
    };
    
    QueryResult(const std::vector<uint32_t>& archetypeIndices, ArchetypeManager* manager)
        : m_archetypeIndices(archetypeIndices), m_archetypeManager(manager) {}
    
    // Iterator for efficient entity traversal
    class Iterator {
    public:
        Iterator(QueryResult* result, size_t archetypeIdx, size_t entityIdx)
            : m_result(result), m_archetypeIdx(archetypeIdx), m_entityIdx(entityIdx) {
            AdvanceToValid();
        }
        
        Iterator& operator++() {
            m_entityIdx++;
            AdvanceToValid();
            return *this;
        }
        
        bool operator!=(const Iterator& other) const {
            return m_archetypeIdx != other.m_archetypeIdx || m_entityIdx != other.m_entityIdx;
        }
        
        EntityData operator*() const {
            Archetype* archetype = m_result->m_archetypeManager->GetArchetype(
                m_result->m_archetypeIndices[m_archetypeIdx]);
            const auto& entities = archetype->GetEntities();
            
            return EntityData{
                entities[m_entityIdx],
                m_result->m_archetypeIndices[m_archetypeIdx],
                static_cast<uint32_t>(m_entityIdx)
            };
        }
        
    private:
        void AdvanceToValid() {
            while (m_archetypeIdx < m_result->m_archetypeIndices.size()) {
                Archetype* archetype = m_result->m_archetypeManager->GetArchetype(
                    m_result->m_archetypeIndices[m_archetypeIdx]);
                
                if (archetype && m_entityIdx < archetype->GetEntityCount()) {
                    return;
                }
                
                m_archetypeIdx++;
                m_entityIdx = 0;
            }
        }
        
        QueryResult* m_result;
        size_t m_archetypeIdx;
        size_t m_entityIdx;
    };
    
    Iterator begin() { return Iterator(this, 0, 0); }
    Iterator end() { return Iterator(this, m_archetypeIndices.size(), 0); }
    
    // Count entities matching query
    size_t Count() const {
        size_t count = 0;
        for (uint32_t archetypeIdx : m_archetypeIndices) {
            Archetype* archetype = m_archetypeManager->GetArchetype(archetypeIdx);
            if (archetype) {
                count += archetype->GetEntityCount();
            }
        }
        return count;
    }
    
    // Get all matching archetypes
    const std::vector<uint32_t>& GetArchetypeIndices() const { return m_archetypeIndices; }
    
private:
    std::vector<uint32_t> m_archetypeIndices;
    ArchetypeManager* m_archetypeManager;
};

// Query builder for finding entities with specific component combinations
class EntityQuery {
public:
    EntityQuery(EntityManager* manager);
    
    // Add required component types
    template<typename T>
    EntityQuery& With() {
        ComponentTypeID typeID = ComponentRegistry::Instance().GetComponentTypeID<T>();
        if (typeID != INVALID_COMPONENT_TYPE) {
            m_requiredMask.set(typeID);
        }
        return *this;
    }
    
    // Add excluded component types
    template<typename T>
    EntityQuery& Without() {
        ComponentTypeID typeID = ComponentRegistry::Instance().GetComponentTypeID<T>();
        if (typeID != INVALID_COMPONENT_TYPE) {
            m_excludedMask.set(typeID);
        }
        return *this;
    }
    
    // Add component filter
    template<typename T>
    EntityQuery& Where(ComponentFilter<T> filter) {
        ComponentTypeID typeID = ComponentRegistry::Instance().GetComponentTypeID<T>();
        if (typeID != INVALID_COMPONENT_TYPE) {
            m_filters[typeID] = [filter](const void* component) {
                return filter(*static_cast<const T*>(component));
            };
        }
        return *this;
    }
    
    // Execute query and return result
    QueryResult Execute();
    
    // Execute query with callback (avoids allocation)
    void ForEach(std::function<void(EntityID)> callback);
    
    // Execute with component access
    template<typename T>
    void ForEach(std::function<void(EntityID, T&)> callback) {
        QueryResult result = Execute();
        
        for (auto entityData : result) {
            Archetype* archetype = m_entityManager->GetArchetypeManager().GetArchetype(entityData.archetypeIndex);
            if (archetype) {
                T* component = archetype->GetComponent<T>(entityData.row);
                if (component) {
                    callback(entityData.entity, *component);
                }
            }
        }
    }
    
    // Execute with multiple component access
    template<typename... Components>
    void ForEach(std::function<void(EntityID, Components&...)> callback) {
        QueryResult result = Execute();
        
        for (auto entityData : result) {
            Archetype* archetype = m_entityManager->GetArchetypeManager().GetArchetype(entityData.archetypeIndex);
            if (archetype) {
                // Get all components
                std::tuple<Components*...> componentPtrs{
                    archetype->GetComponent<Components>(entityData.row)...
                };
                
                // Check all components are valid
                bool allValid = ((std::get<Components*>(componentPtrs) != nullptr) && ...);
                
                if (allValid) {
                    std::apply([&callback, &entityData](Components*... comps) {
                        callback(entityData.entity, *comps...);
                    }, componentPtrs);
                }
            }
        }
    }
    
    // Execute with multiple component access
    template<typename T1, typename T2>
    void ForEach(std::function<void(EntityID, T1&, T2&)> callback) {
        QueryResult result = Execute();
        
        for (auto entityData : result) {
            Archetype* archetype = m_entityManager->GetArchetypeManager().GetArchetype(entityData.archetypeIndex);
            if (archetype) {
                T1* comp1 = archetype->GetComponent<T1>(entityData.row);
                T2* comp2 = archetype->GetComponent<T2>(entityData.row);
                if (comp1 && comp2) {
                    callback(entityData.entity, *comp1, *comp2);
                }
            }
        }
    }
    
    template<typename T1, typename T2, typename T3>
    void ForEach(std::function<void(EntityID, T1&, T2&, T3&)> callback) {
        QueryResult result = Execute();
        
        for (auto entityData : result) {
            Archetype* archetype = m_entityManager->GetArchetypeManager().GetArchetype(entityData.archetypeIndex);
            if (archetype) {
                T1* comp1 = archetype->GetComponent<T1>(entityData.row);
                T2* comp2 = archetype->GetComponent<T2>(entityData.row);
                T3* comp3 = archetype->GetComponent<T3>(entityData.row);
                if (comp1 && comp2 && comp3) {
                    callback(entityData.entity, *comp1, *comp2, *comp3);
                }
            }
        }
    }
    
    // Get first matching entity
    EntityID First();
    
    // Count matching entities
    size_t Count();
    
    // Clear query parameters
    void Clear() {
        m_requiredMask.reset();
        m_excludedMask.reset();
        m_filters.clear();
    }
    
private:
    EntityManager* m_entityManager;
    ComponentMask m_requiredMask;
    ComponentMask m_excludedMask;
    std::unordered_map<ComponentTypeID, std::function<bool(const void*)>> m_filters;
    
    bool PassesFilters(Archetype* archetype, uint32_t row) const;
};

// Query factory for common queries
class Query {
public:
    static EntityQuery All(EntityManager* manager) {
        return EntityQuery(manager);
    }
    
    template<typename T>
    static EntityQuery With(EntityManager* manager) {
        return EntityQuery(manager).With<T>();
    }
    
    template<typename T1, typename T2>
    static EntityQuery With(EntityManager* manager) {
        return EntityQuery(manager).With<T1>().With<T2>();
    }
    
    template<typename T1, typename T2, typename T3>
    static EntityQuery With(EntityManager* manager) {
        return EntityQuery(manager).With<T1>().With<T2>().With<T3>();
    }
};

} // namespace BGE