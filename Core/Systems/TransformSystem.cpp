#include "TransformSystem.h"
#include "../Entity.h"
#include "../ECS/EntityManager.h"
#include "../ECS/EntityQuery.h"
#include "../Logger.h"

namespace BGE {

void TransformSystem::Initialize() {
    BGE_LOG_INFO("TransformSystem", "Initializing Transform System");
    CacheTransformEntities();
}

void TransformSystem::Update(float deltaTime) {
    (void)deltaTime; // Suppress unused parameter warning
    if (!IsEnabled()) return;
    
    // Update all transform hierarchies
    for (Entity* entity : m_transformEntities) {
        if (!entity->IsActive()) continue;
        
        TransformComponent* transform = entity->GetComponent<TransformComponent>();
        if (!transform || transform->parent != INVALID_ENTITY) {
            continue; // Skip if no transform or has parent (will be updated by parent)
        }
        
        // Update root entities and their children
        UpdateTransformHierarchy(entity);
    }
}

void TransformSystem::Shutdown() {
    BGE_LOG_INFO("TransformSystem", "Shutting down Transform System");
    m_transformEntities.clear();
}

void TransformSystem::UpdateTransformHierarchy(Entity* entity) {
    if (!entity) return;
    
    TransformComponent* transform = entity->GetComponent<TransformComponent>();
    if (!transform) return;
    
    // Calculate world transform
    Matrix4 worldTransform = CalculateWorldTransform(transform, entity);
    transform->worldTransform = worldTransform;
    
    // Update children
    UpdateChildTransforms(entity, worldTransform);
}

Matrix4 TransformSystem::CalculateWorldTransform(const TransformComponent* transform, Entity* entity) {
    (void)entity; // Suppress unused parameter warning
    
    // Use the new GetLocalTransform method which uses 3D rotation
    Matrix4 localTransform = transform->GetLocalTransform();
    
    // If has parent, multiply by parent's world transform
    if (transform->parent != INVALID_ENTITY) {
        auto& entityManager = EntityManager::Instance();
        TransformComponent* parentTransform = entityManager.GetComponent<TransformComponent>(transform->parent);
        if (parentTransform) {
            return parentTransform->worldTransform * localTransform;
        }
    }
    
    return localTransform;
}

void TransformSystem::CacheTransformEntities() {
    m_transformEntities.clear();
    auto& entityManager = EntityManager::Instance();
    
    // Query all entities with transform component and create Entity wrappers
    EntityQuery query(&entityManager);
    query.With<TransformComponent>().ForEach([&](EntityID id) {
        // For compatibility, create Entity wrapper objects
        auto entity = new Entity(id.id, entityManager.GetEntityName(id));
        m_transformEntities.push_back(entity);
    });
    
    BGE_LOG_DEBUG("TransformSystem", "Cached " + std::to_string(m_transformEntities.size()) + " transform entities");
}

void TransformSystem::UpdateChildTransforms(Entity* parent, const Matrix4& parentWorldTransform) {
    if (!parent) return;
    
    auto& entityManager = EntityManager::Instance();
    EntityID parentId(parent->GetID());
    TransformComponent* parentTransform = entityManager.GetComponent<TransformComponent>(parentId);
    if (!parentTransform) return;
    
    // Update all children
    for (EntityID childID : parentTransform->children) {
        if (!entityManager.IsEntityValid(childID)) continue;
        
        TransformComponent* childTransform = entityManager.GetComponent<TransformComponent>(childID);
        if (!childTransform) continue;
        
        // Calculate child's world transform using 3D rotation
        Matrix4 childLocalTransform = childTransform->GetLocalTransform();
        childTransform->worldTransform = parentWorldTransform * childLocalTransform;
        
        // Recursively update this child's children (create temporary Entity wrapper)
        Entity tempChild(childID.id, "");
        UpdateChildTransforms(&tempChild, childTransform->worldTransform);
    }
}

} // namespace BGE