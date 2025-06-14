#include "TransformSystem.h"
#include "../Entity.h"
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
        if (!transform || transform->parent != INVALID_ENTITY_ID) {
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
    Matrix4 localTransform = Matrix4::CreateIdentity();
    
    // Apply scale
    localTransform = Matrix4::Scale(transform->scale) * localTransform;
    
    // Apply rotation
    localTransform = Matrix4::RotationZ(transform->rotation) * localTransform;
    
    // Apply translation
    localTransform = Matrix4::Translation(transform->position) * localTransform;
    
    // If has parent, multiply by parent's world transform
    if (transform->parent != INVALID_ENTITY_ID) {
        Entity* parentEntity = EntityManager::Instance().GetEntity(transform->parent);
        if (parentEntity) {
            TransformComponent* parentTransform = parentEntity->GetComponent<TransformComponent>();
            if (parentTransform) {
                return parentTransform->worldTransform * localTransform;
            }
        }
    }
    
    return localTransform;
}

void TransformSystem::CacheTransformEntities() {
    m_transformEntities = EntityManager::Instance().GetEntitiesWithComponent<TransformComponent>();
    BGE_LOG_DEBUG("TransformSystem", "Cached " + std::to_string(m_transformEntities.size()) + " transform entities");
}

void TransformSystem::UpdateChildTransforms(Entity* parent, const Matrix4& parentWorldTransform) {
    if (!parent) return;
    
    TransformComponent* parentTransform = parent->GetComponent<TransformComponent>();
    if (!parentTransform) return;
    
    // Update all children
    for (EntityID childID : parentTransform->children) {
        Entity* child = EntityManager::Instance().GetEntity(childID);
        if (!child || !child->IsActive()) continue;
        
        TransformComponent* childTransform = child->GetComponent<TransformComponent>();
        if (!childTransform) continue;
        
        // Calculate child's world transform
        Matrix4 childLocalTransform = Matrix4::CreateIdentity();
        childLocalTransform = Matrix4::Scale(childTransform->scale) * childLocalTransform;
        childLocalTransform = Matrix4::RotationZ(childTransform->rotation) * childLocalTransform;
        childLocalTransform = Matrix4::Translation(childTransform->position) * childLocalTransform;
        
        childTransform->worldTransform = parentWorldTransform * childLocalTransform;
        
        // Recursively update this child's children
        UpdateChildTransforms(child, childTransform->worldTransform);
    }
}

} // namespace BGE