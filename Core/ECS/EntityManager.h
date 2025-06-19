#pragma once

#include "EntityID.h"
#include "ArchetypeManager.h"
#include "ComponentRegistry.h"
#include "ECSResult.h"
#include "../Logger.h"
#include <vector>
#include <queue>
#include <unordered_map>
#include <memory>
#include <string>
#include <shared_mutex>
#include <atomic>
#include <iostream>

namespace BGE {

// Forward declaration
class Entity;

class EntityManager {
public:
    static EntityManager& Instance() {
        static EntityManager instance;
        return instance;
    }
    
    // Entity creation
    EntityID CreateEntity(const std::string& name = "");
    
    // Entity destruction
    void DestroyEntity(EntityID entity);
    bool IsEntityValid(EntityID entity) const;
    bool IsEntityValidUnsafe(EntityID entity) const; // Internal use only, no locking
    
    // Component operations
    template<typename T>
    ECSResult<T*> AddComponent(EntityID entity, T&& component) {
        std::unique_lock lock(m_mutex);
        m_statComponentAdds.fetch_add(1, std::memory_order_relaxed);
        
        // Validate entity without recursive locking
        if (!IsEntityValidUnsafe(entity)) {
            BGE_LOG_ERROR("EntityManager", "Cannot add component to invalid entity");
            return ECSResult<T*>(ECSErrorInfo(ECSError::InvalidEntity, "Cannot add component to invalid entity"));
        }
        
        // Validate component data
        auto validationResult = ECSValidator::ValidateComponentData(component);
        if (!validationResult) {
            BGE_LOG_ERROR("EntityManager", "Component validation failed: " + validationResult.GetError().message);
            return ECSResult<T*>(ECSErrorInfo(ECSError::ValidationError, "Component validation failed", validationResult.GetError().message));
        }
        
        ComponentTypeID typeID = ComponentRegistry::Instance().GetComponentTypeID<T>();
        if (typeID == INVALID_COMPONENT_TYPE) {
            // Auto-register component type
            std::cout << "EntityManager::AddComponent: Component type not registered, auto-registering: " << typeid(T).name() << std::endl;
            typeID = ComponentRegistry::Instance().RegisterComponent<T>(typeid(T).name());
            if (typeID == INVALID_COMPONENT_TYPE) {
                BGE_LOG_ERROR("EntityManager", "Failed to register component type");
                return ECSResult<T*>(ECSErrorInfo(ECSError::InvalidComponent, "Failed to register component type"));
            }
        } else {
            std::cout << "EntityManager::AddComponent: Found registered component type with ID: " << typeID << " for " << typeid(T).name() << std::endl;
        }
        
        // Validate component type
        if (!ECSValidator::IsValidComponentType(typeID)) {
            BGE_LOG_ERROR("EntityManager", "Component type ID exceeds maximum: " + std::to_string(typeID));
            return ECSResult<T*>(ECSErrorInfo(ECSError::InvalidComponent, "Component type ID exceeds maximum", std::to_string(typeID)));
        }
        
        EntityRecord& record = GetOrCreateRecord(entity);
        
        // Get current archetype
        Archetype* currentArchetype = m_archetypeManager.GetArchetype(record.archetypeIndex);
        
        // Check if component already exists
        if (currentArchetype && currentArchetype->HasComponent(typeID)) {
            BGE_LOG_WARNING("EntityManager", "Entity already has component of this type");
            T* componentPtr = currentArchetype->GetComponent<T>(record.row);
            return ECSResult<T*>(componentPtr);
        }
        
        // Find new archetype
        uint32_t newArchetypeIndex = m_archetypeManager.GetArchetypeAfterAdd(record.archetypeIndex, typeID);
        if (newArchetypeIndex == UINT32_MAX) {
            BGE_LOG_ERROR("EntityManager", "Failed to find or create archetype for component type " + std::to_string(typeID));
            return ECSResult<T*>(ECSErrorInfo(ECSError::ArchetypeLimitReached, "Failed to find or create archetype", "Component type: " + std::to_string(typeID)));
        }
        
        Archetype* newArchetype = m_archetypeManager.GetArchetype(newArchetypeIndex);
        if (!newArchetype) {
            BGE_LOG_ERROR("EntityManager", "Failed to get archetype at index " + std::to_string(newArchetypeIndex));
            return ECSResult<T*>(ECSErrorInfo(ECSError::InvalidOperation, "Failed to get archetype", "Index: " + std::to_string(newArchetypeIndex)));
        }
        
        // Add to new archetype
        uint32_t newRow = newArchetype->AddEntity(entity);
        
        // Move existing components
        if (currentArchetype && record.IsValid()) {
            MoveEntityComponents(entity, record, currentArchetype, newArchetype, newRow);
            
            // Remove from old archetype
            auto removeResult = currentArchetype->RemoveEntity(record.row);
            if (removeResult) {
                EntityID movedEntity = removeResult.GetValue();
                if (movedEntity != entity && movedEntity.IsValid()) {
                    // Update the moved entity's record
                    m_entityRecords[movedEntity.GetIndex()].row = record.row;
                }
            }
        }
        
        // Set the new component
        newArchetype->SetComponent<T>(newRow, std::forward<T>(component));
        
        // Update record
        record.archetypeIndex = newArchetypeIndex;
        record.row = newRow;
        
        T* componentPtr = newArchetype->GetComponent<T>(newRow);
        return ECSResult<T*>(componentPtr);
    }
    
    template<typename T>
    ECSResult<bool> RemoveComponent(EntityID entity) {
        std::unique_lock lock(m_mutex);
        m_statComponentRemoves.fetch_add(1, std::memory_order_relaxed);
        
        if (!IsEntityValidUnsafe(entity)) {
            return ECSResult<bool>(ECSErrorInfo(ECSError::InvalidEntity, "Invalid entity"));
        }
        
        ComponentTypeID typeID = ComponentRegistry::Instance().GetComponentTypeID<T>();
        if (typeID == INVALID_COMPONENT_TYPE) {
            return ECSResult<bool>(ECSErrorInfo(ECSError::InvalidComponent, "Unknown component type"));
        }
        
        EntityRecord& record = m_entityRecords[entity.GetIndex()];
        if (!record.IsValid()) {
            return ECSResult<bool>(ECSErrorInfo(ECSError::InvalidEntity, "Invalid entity record"));
        }
        
        // Get current archetype
        Archetype* currentArchetype = m_archetypeManager.GetArchetype(record.archetypeIndex);
        if (!currentArchetype || !currentArchetype->HasComponent(typeID)) {
            return ECSResult<bool>(ECSErrorInfo(ECSError::ComponentNotFound, "Entity does not have component"));
        }
        
        // Find new archetype
        uint32_t newArchetypeIndex = m_archetypeManager.GetArchetypeAfterRemove(record.archetypeIndex, typeID);
        Archetype* newArchetype = m_archetypeManager.GetArchetype(newArchetypeIndex);
        
        if (!newArchetype) {
            return ECSResult<bool>(ECSErrorInfo(ECSError::InvalidOperation, "Failed to get target archetype"));
        }
        
        // Add to new archetype
        uint32_t newRow = newArchetype->AddEntity(entity);
        
        // Move components (except the one being removed)
        MoveEntityComponents(entity, record, currentArchetype, newArchetype, newRow, typeID);
        
        // Remove from old archetype
        auto removeResult = currentArchetype->RemoveEntity(record.row);
        if (removeResult) {
            EntityID movedEntity = removeResult.GetValue();
            if (movedEntity != entity && movedEntity.IsValid()) {
                // Update the moved entity's record
                m_entityRecords[movedEntity.GetIndex()].row = record.row;
            }
        }
        
        // Update record
        record.archetypeIndex = newArchetypeIndex;
        record.row = newRow;
        
        return ECSResult<bool>(true);
    }
    
    template<typename T>
    T* GetComponent(EntityID entity) {
        std::shared_lock lock(m_mutex);
        if (!IsEntityValidUnsafe(entity)) {
            return nullptr;
        }
        
        const EntityRecord& record = m_entityRecords[entity.GetIndex()];
        if (!record.IsValid()) {
            return nullptr;
        }
        
        Archetype* archetype = m_archetypeManager.GetArchetype(record.archetypeIndex);
        if (!archetype) {
            return nullptr;
        }
        
        T* component = archetype->GetComponent<T>(record.row);
        return component;
    }
    
    template<typename T>
    bool HasComponent(EntityID entity) const {
        std::shared_lock lock(m_mutex);
        if (!IsEntityValidUnsafe(entity)) return false;
        
        const EntityRecord& record = m_entityRecords[entity.GetIndex()];
        if (!record.IsValid()) return false;
        
        const Archetype* archetype = m_archetypeManager.GetArchetype(record.archetypeIndex);
        return archetype && archetype->HasComponent<T>();
    }
    
    // Entity info
    const std::string& GetEntityName(EntityID entity) const;
    void SetEntityName(EntityID entity, const std::string& name);
    size_t GetEntityCount() const { return m_aliveEntityCount; }
    
    // Query support
    ArchetypeManager& GetArchetypeManager() { return m_archetypeManager; }
    const ArchetypeManager& GetArchetypeManager() const { return m_archetypeManager; }
    
    // Clear all entities
    void Clear();
    
    // Get entity generation (for debugging)
    uint32_t GetEntityGeneration(EntityID entity) const {
        return entity.GetGeneration();
    }
    
    // Legacy compatibility methods
    std::vector<EntityID> GetAllEntityIDs() const;
    Entity* GetEntity(uint32_t legacyId);
    std::vector<Entity*> GetAllEntities();
    
    // Note: Legacy compatibility is handled by LegacyEntityWrapper and LegacyEntityManagerAdapter
    
private:
    EntityManager() {
        // Reserve space for initial entities
        m_entityGenerations.reserve(1024);
        m_entityRecords.reserve(1024);
        m_entityNames.reserve(1024);
    }
    
    ~EntityManager(); // Defined in .cpp to avoid incomplete type issues
    
    EntityRecord& GetOrCreateRecord(EntityID entity);
    void MoveEntityComponents(EntityID entity, const EntityRecord& oldRecord, 
                            Archetype* oldArchetype, Archetype* newArchetype, 
                            uint32_t newRow, ComponentTypeID skipType = INVALID_COMPONENT_TYPE);
    
    // Entity storage
    std::vector<uint32_t> m_entityGenerations;
    std::vector<EntityRecord> m_entityRecords;
    std::vector<std::string> m_entityNames;
    std::queue<uint32_t> m_freeEntityIndices;
    size_t m_aliveEntityCount = 0;
    
    // Archetype management
    ArchetypeManager m_archetypeManager;
    
    // Note: Legacy entity storage moved to LegacyEntityManagerAdapter
    
    // Thread safety
    mutable std::shared_mutex m_mutex;
    
    // Performance statistics
    mutable std::atomic<uint64_t> m_statEntityCreations{0};
    mutable std::atomic<uint64_t> m_statEntityDestructions{0};
    mutable std::atomic<uint64_t> m_statComponentAdds{0};
    mutable std::atomic<uint64_t> m_statComponentRemoves{0};
};

} // namespace BGE