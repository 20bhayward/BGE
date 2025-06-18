#include "EntityManager.h"
#include "../Logger.h"
#include "../Entity.h"  // Must come after EntityManager.h to avoid circular dependency

namespace BGE {

EntityManager::~EntityManager() {
    // Destructor
}

EntityID EntityManager::CreateEntity(const std::string& name) {
    std::unique_lock lock(m_mutex);
    BGE_LOG_DEBUG("EntityManager", "Creating entity: " + name);
    uint32_t index;
    uint32_t generation;
    
    if (!m_freeEntityIndices.empty()) {
        // Reuse a free index
        index = m_freeEntityIndices.front();
        m_freeEntityIndices.pop();
        generation = m_entityGenerations[index];
    } else {
        // Allocate new index
        index = static_cast<uint32_t>(m_entityGenerations.size());
        generation = 0;
        
        // Check if we've reached the entity limit
        if (!ECSValidator::IsValidEntityIndex(index)) {
            BGE_LOG_ERROR("EntityManager", "Entity limit reached. Cannot create more entities.");
            return EntityID::Invalid();
        }
        
        // Grow arrays
        m_entityGenerations.push_back(generation);
        m_entityRecords.emplace_back();
        m_entityNames.emplace_back();
    }
    
    // Create entity ID
    EntityID entity(index, generation);
    
    // Initialize record (starts in empty archetype)
    m_entityRecords[index] = EntityRecord{0, 0};
    m_entityNames[index] = name;
    
    // Add to empty archetype
    Archetype* emptyArchetype = m_archetypeManager.GetArchetype(0);
    if (emptyArchetype) {
        uint32_t row = emptyArchetype->AddEntity(entity);
        m_entityRecords[index].row = row;
    }
    
    m_aliveEntityCount++;
    m_statEntityCreations.fetch_add(1, std::memory_order_relaxed);
    
    return entity;
}

void EntityManager::DestroyEntity(EntityID entity) {
    std::unique_lock lock(m_mutex);
    if (!IsEntityValidUnsafe(entity)) return;
    
    uint32_t index = entity.GetIndex();
    EntityRecord& record = m_entityRecords[index];
    
    if (record.IsValid()) {
        // Remove from archetype
        Archetype* archetype = m_archetypeManager.GetArchetype(record.archetypeIndex);
        if (archetype) {
            auto removeResult = archetype->RemoveEntity(record.row);
            if (!removeResult) {
                BGE_LOG_ERROR("EntityManager", "Failed to remove entity from archetype: " + removeResult.GetError().message);
                // Continue with cleanup anyway
            } else {
                EntityID movedEntity = removeResult.GetValue();
                
                // Update moved entity's record if different
                if (movedEntity != entity && movedEntity.IsValid()) {
                    m_entityRecords[movedEntity.GetIndex()].row = record.row;
                }
            }
        }
    }
    
    // Increment generation
    m_entityGenerations[index]++;
    
    // Clear record
    record = EntityRecord();
    m_entityNames[index].clear();
    
    // Add to free list
    m_freeEntityIndices.push(index);
    
    m_aliveEntityCount--;
    m_statEntityDestructions.fetch_add(1, std::memory_order_relaxed);
}

bool EntityManager::IsEntityValid(EntityID entity) const {
    std::shared_lock lock(m_mutex);
    return IsEntityValidUnsafe(entity);
}

bool EntityManager::IsEntityValidUnsafe(EntityID entity) const {
    uint32_t index = entity.GetIndex();
    if (index >= m_entityGenerations.size()) {
        return false;
    }
    
    return m_entityGenerations[index] == entity.GetGeneration();
}

const std::string& EntityManager::GetEntityName(EntityID entity) const {
    static const std::string empty;
    std::shared_lock lock(m_mutex);
    
    if (!IsEntityValidUnsafe(entity)) {
        return empty;
    }
    
    return m_entityNames[entity.GetIndex()];
}

void EntityManager::SetEntityName(EntityID entity, const std::string& name) {
    std::unique_lock lock(m_mutex);
    
    if (!IsEntityValidUnsafe(entity)) return;
    
    m_entityNames[entity.GetIndex()] = name;
}

void EntityManager::Clear() {
    std::unique_lock lock(m_mutex);
    
    // Destroy all entities (using unsafe version to avoid recursive lock)
    for (size_t i = 0; i < m_entityGenerations.size(); ++i) {
        EntityID entity(static_cast<uint32_t>(i), m_entityGenerations[i]);
        if (IsEntityValidUnsafe(entity)) {
            // Inline destruction logic to avoid recursive lock
            uint32_t index = entity.GetIndex();
            EntityRecord& record = m_entityRecords[index];
            
            if (record.IsValid()) {
                // Remove from archetype
                Archetype* archetype = m_archetypeManager.GetArchetype(record.archetypeIndex);
                if (archetype) {
                    auto removeResult = archetype->RemoveEntity(record.row);
                    if (!removeResult) {
                        BGE_LOG_ERROR("EntityManager", "Failed to remove entity from archetype during clear: " + removeResult.GetError().message);
                        // Continue with cleanup anyway
                    } else {
                        EntityID movedEntity = removeResult.GetValue();
                        
                        // Update moved entity's record if different
                        if (movedEntity != entity && movedEntity.IsValid()) {
                            m_entityRecords[movedEntity.GetIndex()].row = record.row;
                        }
                    }
                }
            }
            
            // Increment generation
            m_entityGenerations[index]++;
            
            // Clear record
            record = EntityRecord();
            m_entityNames[index].clear();
            
            m_aliveEntityCount--;
            m_statEntityDestructions.fetch_add(1, std::memory_order_relaxed);
        }
    }
    
    // Clear all data
    m_entityGenerations.clear();
    m_entityRecords.clear();
    m_entityNames.clear();
    while (!m_freeEntityIndices.empty()) {
        m_freeEntityIndices.pop();
    }
    m_aliveEntityCount = 0;
    
    // Reset archetype manager
    m_archetypeManager = ArchetypeManager();
    
    // Legacy entity cleanup handled by LegacyEntityManagerAdapter
}

EntityRecord& EntityManager::GetOrCreateRecord(EntityID entity) {
    uint32_t index = entity.GetIndex();
    
    // Ensure arrays are large enough
    while (index >= m_entityRecords.size()) {
        m_entityGenerations.push_back(0);
        m_entityRecords.emplace_back();
        m_entityNames.emplace_back();
    }
    
    return m_entityRecords[index];
}

void EntityManager::MoveEntityComponents(EntityID /*entity*/, const EntityRecord& oldRecord,
                                       Archetype* oldArchetype, Archetype* newArchetype,
                                       uint32_t newRow, ComponentTypeID skipType) {
    if (!oldArchetype || !newArchetype) return;
    
    // Get component types from old archetype
    const auto& componentTypes = oldArchetype->GetComponentTypes();
    
    // Copy each component to new archetype
    for (ComponentTypeID typeID : componentTypes) {
        if (typeID == skipType) continue; // Skip if removing this component
        
        if (!newArchetype->HasComponent(typeID)) continue; // Skip if new archetype doesn't have it
        
        // Get component info
        const ComponentInfo* info = ComponentRegistry::Instance().GetComponentInfo(typeID);
        if (!info) continue;
        
        // Get storages
        IComponentStorage* oldStorage = oldArchetype->GetComponentStorage(typeID);
        IComponentStorage* newStorage = newArchetype->GetComponentStorage(typeID);
        
        if (!oldStorage || !newStorage) continue;
        
        // Copy component data
        const void* oldData = oldStorage->GetRaw(oldRecord.row);
        void* newData = newStorage->GetRaw(newRow);
        
        if (oldData && newData) {
            // Use move constructor if available
            info->moveConstructor(newData, const_cast<void*>(oldData));
        }
    }
}

// Legacy compatibility methods
std::vector<EntityID> EntityManager::GetAllEntityIDs() const {
    std::shared_lock lock(m_mutex);
    std::vector<EntityID> entities;
    
    for (size_t i = 0; i < m_entityGenerations.size(); ++i) {
        EntityID entity(static_cast<uint32_t>(i), m_entityGenerations[i]);
        if (IsEntityValidUnsafe(entity)) {
            entities.push_back(entity);
        }
    }
    
    return entities;
}

Entity* EntityManager::GetEntity(uint32_t legacyId) {
    // For now, return nullptr since Entity objects are managed differently
    // This would need to be implemented with a legacy entity cache
    (void)legacyId;
    return nullptr;
}

std::vector<Entity*> EntityManager::GetAllEntities() {
    // For now, return empty since Entity objects are managed differently
    // This would need to be implemented with a legacy entity cache
    return std::vector<Entity*>();
}

} // namespace BGE