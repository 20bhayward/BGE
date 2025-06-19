#pragma once

#include "EntityID.h"
#include "ComponentRegistry.h"
#include "ComponentStorage.h"
#include "PooledComponentStorage.h"
#include "ECSResult.h"
#include "../Logger.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <bitset>
#include <iostream>

namespace BGE {

// Component mask for fast archetype comparison
using ComponentMask = std::bitset<MAX_COMPONENTS>;

// Record of where an entity's components are stored
struct EntityRecord {
    uint32_t archetypeIndex = UINT32_MAX;
    uint32_t row = UINT32_MAX;
    
    bool IsValid() const { return archetypeIndex != UINT32_MAX; }
};

// Archetype represents a unique combination of components
class Archetype {
public:
    Archetype(ComponentMask mask, const std::vector<ComponentTypeID>& types)
        : m_mask(mask), m_componentTypes(types) {
        
        // Sort component types for consistent ordering
        std::sort(m_componentTypes.begin(), m_componentTypes.end());
        
        // Create storage for each component type
        // Storage will be created lazily when components are first added
    }
    
    // Add an entity to this archetype
    uint32_t AddEntity(EntityID entity) {
        if (!entity.IsValid()) {
            BGE_LOG_ERROR("Archetype", "Cannot add invalid entity to archetype");
            return UINT32_MAX;
        }
        
        uint32_t row = static_cast<uint32_t>(m_entities.size());
        
        // Check for overflow
        if (row == UINT32_MAX) {
            BGE_LOG_ERROR("Archetype", "Archetype row limit reached");
            return UINT32_MAX;
        }
        
        m_entities.push_back(entity);
        
        // Initialize storages for all component types
        for (ComponentTypeID typeID : m_componentTypes) {
            // Ensure storage exists
            if (m_componentStorages.find(typeID) == m_componentStorages.end()) {
                // Create storage using component info
                const ComponentInfo* info = ComponentRegistry::Instance().GetComponentInfo(typeID);
                if (info) {
                    // Create generic storage for now
                    // TODO: Add factory to ComponentInfo to create proper typed storage
                    m_componentStorages[typeID] = std::make_unique<GenericComponentStorage>(*info);
                }
            }
            
            // Add default element for this entity
            IComponentStorage* storage = m_componentStorages[typeID].get();
            if (storage) {
                storage->PushDefault();
            }
        }
        
        return row;
    }
    
    // Remove an entity by swapping with last
    ECSResult<EntityID> RemoveEntity(uint32_t row) {
        if (row >= m_entities.size()) {
            return ECSResult<EntityID>(ECSErrorInfo(ECSError::InvalidOperation, "Row index out of bounds", std::to_string(row)));
        }
        
        EntityID movedEntity = m_entities.back();
        
        if (row != m_entities.size() - 1) {
            // Swap with last entity
            m_entities[row] = movedEntity;
            
            // Move components
            for (auto& [typeID, storage] : m_componentStorages) {
                if (storage && storage->Size() > row) {
                    storage->MoveFrom(row, storage->Size() - 1);
                }
            }
        }
        
        m_entities.pop_back();
        
        // Remove last component in each storage
        for (auto& [typeID, storage] : m_componentStorages) {
            if (storage && storage->Size() > 0) {
                storage->Remove(storage->Size() - 1);
            }
        }
        
        return ECSResult<EntityID>(movedEntity);
    }
    
    // Get component storage
    template<typename T>
    ComponentStorage<T>* GetComponentStorage() {
        ComponentTypeID typeID = ComponentRegistry::Instance().GetComponentTypeID<T>();
        
        auto it = m_componentStorages.find(typeID);
        if (it != m_componentStorages.end()) {
            auto* typed = dynamic_cast<TypedComponentStorage<T>*>(it->second.get());
            if (typed) {
                return &typed->GetTypedStorage();
            }
            
            // Storage exists but is wrong type (GenericComponentStorage)
            // Replace it with proper typed storage, preserving existing data
            if (HasComponent(typeID)) {
                // Save old storage size
                size_t oldSize = it->second->Size();
                
                // Create new typed storage
                auto newStorage = CreateStorageForComponent<T>();
                auto* typedNew = dynamic_cast<TypedComponentStorage<T>*>(newStorage.get());
                if (typedNew) {
                    // Ensure new storage has same number of elements
                    auto& storage = typedNew->GetTypedStorage();
                    storage.Reserve(oldSize);
                    for (size_t i = 0; i < oldSize; ++i) {
                        storage.Emplace();
                    }
                    
                    // Replace storage
                    m_componentStorages[typeID] = std::move(newStorage);
                    return &typedNew->GetTypedStorage();
                }
            }
            
            return nullptr;
        }
        
        // Create storage if it doesn't exist and this archetype should have it
        if (HasComponent(typeID)) {
            m_componentStorages[typeID] = CreateStorageForComponent<T>();
            auto* typed = dynamic_cast<TypedComponentStorage<T>*>(m_componentStorages[typeID].get());
            return typed ? &typed->GetTypedStorage() : nullptr;
        }
        
        return nullptr;
    }
    
    // Get type-erased component storage (read-only, doesn't create)
    IComponentStorage* GetComponentStorage(ComponentTypeID typeID) const {
        auto it = m_componentStorages.find(typeID);
        return it != m_componentStorages.end() ? it->second.get() : nullptr;
    }
    
    // Get component by entity row
    template<typename T>
    T* GetComponent(uint32_t row) {
        auto* storage = GetComponentStorage<T>();
        if (!storage) {
            return nullptr;
        }
        if (row >= storage->Size()) {
            return nullptr;
        }
        return &storage->Get(row);
    }
    
    // Set component data
    template<typename T>
    void SetComponent(uint32_t row, T&& component) {
        auto* storage = GetComponentStorage<T>();
        if (!storage) {
            return;
        }
        
        // Storage should already have the element from AddEntity
        if (row >= storage->Size()) {
            // Add missing elements as fallback
            while (storage->Size() <= row) {
                storage->Emplace();
            }
        }
        
        storage->Get(row) = std::forward<T>(component);
    }
    
    // Getters
    const ComponentMask& GetMask() const { return m_mask; }
    const std::vector<ComponentTypeID>& GetComponentTypes() const { return m_componentTypes; }
    const std::vector<EntityID>& GetEntities() const { return m_entities; }
    size_t GetEntityCount() const { return m_entities.size(); }
    
    // Check if archetype has component
    bool HasComponent(ComponentTypeID typeID) const {
        return typeID < MAX_COMPONENTS && m_mask.test(typeID);
    }
    
    template<typename T>
    bool HasComponent() const {
        ComponentTypeID typeID = ComponentRegistry::Instance().GetComponentTypeID<T>();
        return HasComponent(typeID);
    }
    
private:
    ComponentMask m_mask;
    std::vector<ComponentTypeID> m_componentTypes;
    std::vector<EntityID> m_entities;
    mutable std::unordered_map<ComponentTypeID, std::unique_ptr<IComponentStorage>> m_componentStorages;
    
    // Factory function to create storage based on type
    std::unique_ptr<IComponentStorage> CreateStorageForType(std::type_index typeIndex);
    
    // Template version for direct type specification
    template<typename T>
    std::unique_ptr<IComponentStorage> CreateStorageForComponent() {
        // Use regular typed storage for now
        return std::make_unique<TypedComponentStorage<T>>();
    }
};

// Archetype edge for fast archetype transitions
struct ArchetypeEdge {
    uint32_t add = UINT32_MAX;    // Archetype when adding component
    uint32_t remove = UINT32_MAX; // Archetype when removing component
};

} // namespace BGE