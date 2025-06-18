#include "Entity.h"
#include "ECS/EntityManager.h"
#include "ECS/ECSResult.h"
#include "Components.h"

namespace BGE {

Entity::Entity(uint32_t legacyId, const std::string& name)
    : m_legacyId(legacyId), m_name(name) {
}

EntityID Entity::GetECSEntityID() const {
    // Convert legacy ID to ECS EntityID
    // Assume generation 0 for existing entities
    return EntityID(m_legacyId, 0);
}

template<typename T>
T* Entity::GetComponent() {
    return EntityManager::Instance().GetComponent<T>(GetECSEntityID());
}

template<typename T>
T* Entity::AddComponent(T&& component) {
    auto result = EntityManager::Instance().AddComponent<T>(GetECSEntityID(), std::forward<T>(component));
    return result ? result.GetValue() : nullptr;
}

template<typename T>
bool Entity::HasComponent() const {
    return EntityManager::Instance().HasComponent<T>(GetECSEntityID());
}

template<typename T>
void Entity::RemoveComponent() {
    auto result = EntityManager::Instance().RemoveComponent<T>(GetECSEntityID());
    (void)result; // Ignore the result for now - legacy interface doesn't report errors
}

// Explicit template instantiations for components used by UI panels
template TransformComponent* Entity::GetComponent<TransformComponent>();
template NameComponent* Entity::GetComponent<NameComponent>();
template MaterialComponent* Entity::GetComponent<MaterialComponent>();
template SpriteComponent* Entity::GetComponent<SpriteComponent>();
template LightComponent* Entity::GetComponent<LightComponent>();
template RigidbodyComponent* Entity::GetComponent<RigidbodyComponent>();
template VelocityComponent* Entity::GetComponent<VelocityComponent>();
template HealthComponent* Entity::GetComponent<HealthComponent>();

template TransformComponent* Entity::AddComponent<TransformComponent>(TransformComponent&& component);
template NameComponent* Entity::AddComponent<NameComponent>(NameComponent&& component);
template MaterialComponent* Entity::AddComponent<MaterialComponent>(MaterialComponent&& component);
template SpriteComponent* Entity::AddComponent<SpriteComponent>(SpriteComponent&& component);
template LightComponent* Entity::AddComponent<LightComponent>(LightComponent&& component);
template RigidbodyComponent* Entity::AddComponent<RigidbodyComponent>(RigidbodyComponent&& component);
template VelocityComponent* Entity::AddComponent<VelocityComponent>(VelocityComponent&& component);
template HealthComponent* Entity::AddComponent<HealthComponent>(HealthComponent&& component);

template bool Entity::HasComponent<TransformComponent>() const;
template bool Entity::HasComponent<NameComponent>() const;
template bool Entity::HasComponent<MaterialComponent>() const;
template bool Entity::HasComponent<SpriteComponent>() const;
template bool Entity::HasComponent<LightComponent>() const;
template bool Entity::HasComponent<RigidbodyComponent>() const;
template bool Entity::HasComponent<VelocityComponent>() const;
template bool Entity::HasComponent<HealthComponent>() const;

template void Entity::RemoveComponent<TransformComponent>();
template void Entity::RemoveComponent<NameComponent>();
template void Entity::RemoveComponent<MaterialComponent>();
template void Entity::RemoveComponent<SpriteComponent>();
template void Entity::RemoveComponent<LightComponent>();
template void Entity::RemoveComponent<RigidbodyComponent>();
template void Entity::RemoveComponent<VelocityComponent>();
template void Entity::RemoveComponent<HealthComponent>();

} // namespace BGE