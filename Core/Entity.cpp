#include "Entity.h"

namespace BGE {

Entity::Entity(EntityID id, const std::string& name)
    : m_id(id), m_name(name) {
}

EntityManager& EntityManager::Instance() {
    static EntityManager instance;
    return instance;
}

Entity* EntityManager::CreateEntity(const std::string& name) {
    EntityID id = GenerateEntityID();
    auto entity = std::make_unique<Entity>(id, name);
    Entity* ptr = entity.get();
    m_entities[id] = std::move(entity);
    return ptr;
}

void EntityManager::DestroyEntity(EntityID id) {
    m_entities.erase(id);
}

void EntityManager::DestroyEntity(Entity* entity) {
    if (entity) {
        DestroyEntity(entity->GetID());
    }
}

Entity* EntityManager::GetEntity(EntityID id) {
    auto it = m_entities.find(id);
    return it != m_entities.end() ? it->second.get() : nullptr;
}

const Entity* EntityManager::GetEntity(EntityID id) const {
    auto it = m_entities.find(id);
    return it != m_entities.end() ? it->second.get() : nullptr;
}

void EntityManager::Clear() {
    m_entities.clear();
    m_nextEntityID = 1;
}

EntityID EntityManager::GenerateEntityID() {
    return m_nextEntityID++;
}

} // namespace BGE