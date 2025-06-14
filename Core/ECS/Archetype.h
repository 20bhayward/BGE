#pragma once

#include <vector>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <bitset>

namespace BGE {

using EntityID = uint64_t;
using ComponentID = size_t;

// Maximum number of component types
constexpr size_t MAX_COMPONENTS = 128;

// Component type mask for fast comparison
using ComponentMask = std::bitset<MAX_COMPONENTS>;

// Base class for component storage
class IComponentArray {
public:
    virtual ~IComponentArray() = default;
    virtual void RemoveEntity(EntityID entity) = 0;
    virtual void MoveEntity(EntityID from, EntityID to) = 0;
    virtual size_t GetSize() const = 0;
};

// Typed component storage using Structure-of-Arrays layout
template<typename T>
class ComponentArray : public IComponentArray {
public:
    void AddComponent(EntityID entity, T component) {
        size_t index = m_components.size();
        m_components.push_back(std::move(component));
        m_entityToIndex[entity] = index;
        m_indexToEntity[index] = entity;
    }
    
    T* GetComponent(EntityID entity) {
        auto it = m_entityToIndex.find(entity);
        if (it != m_entityToIndex.end()) {
            return &m_components[it->second];
        }
        return nullptr;
    }
    
    void RemoveEntity(EntityID entity) override {
        auto it = m_entityToIndex.find(entity);
        if (it == m_entityToIndex.end()) return;
        
        size_t indexToRemove = it->second;
        size_t lastIndex = m_components.size() - 1;
        
        // Swap with last element
        if (indexToRemove != lastIndex) {
            m_components[indexToRemove] = std::move(m_components[lastIndex]);
            
            // Update mappings
            EntityID lastEntity = m_indexToEntity[lastIndex];
            m_entityToIndex[lastEntity] = indexToRemove;
            m_indexToEntity[indexToRemove] = lastEntity;
        }
        
        m_components.pop_back();
        m_entityToIndex.erase(entity);
        m_indexToEntity.erase(lastIndex);
    }
    
    void MoveEntity(EntityID from, EntityID to) override {
        auto it = m_entityToIndex.find(from);
        if (it != m_entityToIndex.end()) {
            size_t index = it->second;
            m_entityToIndex.erase(from);
            m_entityToIndex[to] = index;
            m_indexToEntity[index] = to;
        }
    }
    
    size_t GetSize() const override { return m_components.size(); }
    
    // Direct access to component array for iteration
    std::vector<T>& GetComponents() { return m_components; }
    const std::vector<T>& GetComponents() const { return m_components; }
    
private:
    std::vector<T> m_components;
    std::unordered_map<EntityID, size_t> m_entityToIndex;
    std::unordered_map<size_t, EntityID> m_indexToEntity;
};

// Archetype represents a unique combination of components
class Archetype {
public:
    Archetype(ComponentMask mask) : m_mask(mask) {}
    
    template<typename T>
    void AddComponent(EntityID entity, T component) {
        ComponentID id = GetComponentID<T>();
        
        if (!m_componentArrays[id]) {
            m_componentArrays[id] = std::make_unique<ComponentArray<T>>();
        }
        
        static_cast<ComponentArray<T>*>(m_componentArrays[id].get())
            ->AddComponent(entity, std::move(component));
        
        m_entities.push_back(entity);
    }
    
    template<typename T>
    T* GetComponent(EntityID entity) {
        ComponentID id = GetComponentID<T>();
        if (m_componentArrays[id]) {
            return static_cast<ComponentArray<T>*>(m_componentArrays[id].get())
                ->GetComponent(entity);
        }
        return nullptr;
    }
    
    void RemoveEntity(EntityID entity) {
        // Remove from all component arrays
        for (auto& [id, array] : m_componentArrays) {
            if (array) {
                array->RemoveEntity(entity);
            }
        }
        
        // Remove from entity list
        m_entities.erase(
            std::remove(m_entities.begin(), m_entities.end(), entity),
            m_entities.end()
        );
    }
    
    const ComponentMask& GetMask() const { return m_mask; }
    const std::vector<EntityID>& GetEntities() const { return m_entities; }
    
    template<typename T>
    ComponentArray<T>* GetComponentArray() {
        ComponentID id = GetComponentID<T>();
        if (m_componentArrays[id]) {
            return static_cast<ComponentArray<T>*>(m_componentArrays[id].get());
        }
        return nullptr;
    }
    
private:
    template<typename T>
    static ComponentID GetComponentID() {
        static ComponentID id = s_nextComponentID++;
        return id;
    }
    
    ComponentMask m_mask;
    std::vector<EntityID> m_entities;
    std::unordered_map<ComponentID, std::unique_ptr<IComponentArray>> m_componentArrays;
    
    inline static ComponentID s_nextComponentID = 0;
};

} // namespace BGE