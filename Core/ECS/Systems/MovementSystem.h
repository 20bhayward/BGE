#pragma once

#include "../System.h"
#include "../../Components.h"
#include "../../Math/Quaternion.h"

namespace BGE {

// System that updates entity positions based on velocity
class MovementSystem : public ComponentSystem<TransformComponent, VelocityComponent> {
public:
    MovementSystem() {
        SetName("MovementSystem");
        SetStage(SystemStage::Update);
        SetPriority(100); // Early in update
    }
    
protected:
    void OnUpdateEntity(EntityID /*entity*/, TransformComponent& transform, VelocityComponent& velocity) override {
        // Update position based on velocity
        transform.position.x += velocity.velocity.x * m_deltaTime;
        transform.position.y += velocity.velocity.y * m_deltaTime;
        transform.position.z += velocity.velocity.z * m_deltaTime;
        
        // Update rotation based on angular velocity
        if (velocity.angular.Length() > 0.0f) {
            // For now, just update 2D rotation from Z angular velocity
            transform.rotation += velocity.angular.z * m_deltaTime;
            
            // Update 3D rotation quaternion
            Quaternion angularDelta = Quaternion::FromEuler(
                velocity.angular.x * m_deltaTime,
                velocity.angular.y * m_deltaTime,
                velocity.angular.z * m_deltaTime
            );
            transform.rotation3D = transform.rotation3D * angularDelta;
        }
    }
    
    void OnUpdate(float deltaTime) override {
        m_deltaTime = deltaTime;
        ComponentSystem::OnUpdate(deltaTime);
    }
    
private:
    float m_deltaTime = 0.0f;
};

} // namespace BGE