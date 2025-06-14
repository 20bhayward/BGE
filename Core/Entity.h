#pragma once

#include <string>
#include <vector>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <algorithm>

namespace BGE {

using EntityID = uint64_t;
constexpr EntityID INVALID_ENTITY_ID = 0;

class Component {
public:
    virtual ~Component() = default;
    
    EntityID GetEntityID() const { return m_entityID; }
    void SetEntityID(EntityID id) { m_entityID = id; }
    
private:
    EntityID m_entityID = INVALID_ENTITY_ID;
};

class Entity {
public:
    Entity(EntityID id, const std::string& name = "");
    ~Entity() = default;
    
    EntityID GetID() const { return m_id; }
    const std::string& GetName() const { return m_name; }
    void SetName(const std::string& name) { m_name = name; }
    
    bool IsActive() const { return m_active; }
    void SetActive(bool active) { m_active = active; }
    
    template<typename T, typename... Args>
    T* AddComponent(Args&&... args) {
        static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");
        
        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        component->SetEntityID(m_id);
        
        T* ptr = component.get();
        auto typeId = std::type_index(typeid(T));
        m_components[typeId] = std::move(component);
        
        return ptr;
    }
    
    template<typename T>
    T* GetComponent() {
        static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");
        
        auto typeId = std::type_index(typeid(T));
        auto it = m_components.find(typeId);
        if (it != m_components.end()) {
            return static_cast<T*>(it->second.get());
        }
        return nullptr;
    }
    
    template<typename T>
    bool HasComponent() const {
        auto typeId = std::type_index(typeid(T));
        return m_components.find(typeId) != m_components.end();
    }
    
    template<typename T>
    void RemoveComponent() {
        auto typeId = std::type_index(typeid(T));
        m_components.erase(typeId);
    }
    
    void RemoveAllComponents() { m_components.clear(); }
    
    size_t GetComponentCount() const { return m_components.size(); }
    
    // Check if entity has component by type index (for queries)
    bool HasComponentType(const std::type_index& type) const {
        return m_components.find(type) != m_components.end();
    }
    
private:
    EntityID m_id;
    std::string m_name;
    bool m_active = true;
    std::unordered_map<std::type_index, std::unique_ptr<Component>> m_components;
};

class EntityManager {
public:
    static EntityManager& Instance();
    
    Entity* CreateEntity(const std::string& name = "");
    void DestroyEntity(EntityID id);
    void DestroyEntity(Entity* entity);
    
    Entity* GetEntity(EntityID id);
    const Entity* GetEntity(EntityID id) const;
    
    template<typename T>
    std::vector<Entity*> GetEntitiesWithComponent() {
        std::vector<Entity*> result;
        for (auto& pair : m_entities) {
            if (pair.second->HasComponent<T>()) {
                result.push_back(pair.second.get());
            }
        }
        return result;
    }
    
    void Clear();
    size_t GetEntityCount() const { return m_entities.size(); }
    
    // Get all entities (for queries)
    const std::unordered_map<EntityID, std::unique_ptr<Entity>>& GetAllEntities() const {
        return m_entities;
    }
    
private:
    EntityManager() = default;
    ~EntityManager() = default;
    
    EntityID GenerateEntityID();
    
    EntityID m_nextEntityID = 1;
    std::unordered_map<EntityID, std::unique_ptr<Entity>> m_entities;
};

} // namespace BGE