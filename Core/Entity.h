#pragma once

#include <string>
#include <vector>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <algorithm>
#include "ECS/EntityID.h"

namespace BGE {

// Forward declaration to avoid circular dependency
class EntityManager;

// Legacy Entity wrapper that delegates to new ECS system
class Entity {
public:
    Entity(uint32_t legacyId, const std::string& name = "");
    ~Entity() = default;
    
    // Legacy interface
    uint32_t GetID() const { return m_legacyId; }
    const std::string& GetName() const { return m_name; }
    void SetName(const std::string& name) { m_name = name; }
    
    bool IsActive() const { return m_active; }
    void SetActive(bool active) { m_active = active; }
    
    // Component access using new ECS system
    template<typename T>
    T* GetComponent();
    
    template<typename T>
    T* AddComponent(T&& component);
    
    template<typename T>
    bool HasComponent() const;
    
    template<typename T>
    void RemoveComponent();
    
    void RemoveAllComponents() { /* Implement if needed */ }
    
    size_t GetComponentCount() const { return 0; /* Could be implemented by querying ECS */ }
    
    // Get the new ECS EntityID
    EntityID GetECSEntityID() const;
    
private:
    uint32_t m_legacyId;
    std::string m_name;
    bool m_active = true;
};

// Legacy EntityManager - use BGE::EntityManager from ECS/EntityManager.h instead
// This declaration is kept for backward compatibility only

} // namespace BGE