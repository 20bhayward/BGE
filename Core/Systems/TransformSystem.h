#pragma once

#include "ISystem.h"
#include "../Components.h"
#include <vector>

namespace BGE {

// Example system that updates transform hierarchies
class TransformSystem : public ISystem {
public:
    TransformSystem() = default;
    ~TransformSystem() override = default;
    
    // ISystem implementation
    void Initialize() override;
    void Update(float deltaTime) override;
    void Shutdown() override;
    
    std::vector<ComponentType> GetRequiredComponents() const override {
        return { std::type_index(typeid(TransformComponent)) };
    }
    
    std::string GetName() const override { return "TransformSystem"; }
    SystemPriority GetPriority() const override { return SystemPriority::Movement; }
    
    // Transform-specific operations
    void UpdateTransformHierarchy(Entity* entity);
    Matrix4 CalculateWorldTransform(const TransformComponent* transform, Entity* entity);
    
private:
    std::vector<Entity*> m_transformEntities;
    
    void CacheTransformEntities();
    void UpdateChildTransforms(Entity* parent, const Matrix4& parentWorldTransform);
};

} // namespace BGE