#pragma once

#include "RigidBody.h"
#include <vector>
#include <memory>

namespace BGE {

class PhysicsWorld {
public:
    PhysicsWorld();
    ~PhysicsWorld();
    
    void Update(float deltaTime);
    
    // RigidBody management
    RigidBody* CreateRigidBody();
    void DestroyRigidBody(RigidBody* body);
    
    // World properties
    Vector2 GetGravity() const { return m_gravity; }
    void SetGravity(const Vector2& gravity) { m_gravity = gravity; }
    
    // Collision detection (simplified)
    void CheckCollisions();

private:
    std::vector<std::unique_ptr<RigidBody>> m_bodies;
    Vector2 m_gravity;
    
    void ApplyGravity(float deltaTime);
    void IntegrateForces(float deltaTime);
    void SolveConstraints();
    void IntegrateVelocities(float deltaTime);
};

} // namespace BGE