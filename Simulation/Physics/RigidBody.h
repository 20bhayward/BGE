#pragma once

#include "../../Core/Math/Vector2.h"

namespace BGE {

class RigidBody {
public:
    RigidBody();
    ~RigidBody();
    
    // Transform
    Vector2 GetPosition() const { return m_position; }
    void SetPosition(const Vector2& position) { m_position = position; }
    
    float GetRotation() const { return m_rotation; }
    void SetRotation(float rotation) { m_rotation = rotation; }
    
    // Physics properties
    Vector2 GetVelocity() const { return m_velocity; }
    void SetVelocity(const Vector2& velocity) { m_velocity = velocity; }
    
    float GetAngularVelocity() const { return m_angularVelocity; }
    void SetAngularVelocity(float angularVelocity) { m_angularVelocity = angularVelocity; }
    
    float GetMass() const { return m_mass; }
    void SetMass(float mass);
    
    float GetInertia() const { return m_inertia; }
    void SetInertia(float inertia) { m_inertia = inertia; }
    
    // Material properties
    float GetRestitution() const { return m_restitution; }
    void SetRestitution(float restitution) { m_restitution = restitution; }
    
    float GetFriction() const { return m_friction; }
    void SetFriction(float friction) { m_friction = friction; }
    
    // Force application
    void ApplyForce(const Vector2& force);
    void ApplyForceAtPoint(const Vector2& force, const Vector2& point);
    void ApplyImpulse(const Vector2& impulse);
    void ApplyImpulseAtPoint(const Vector2& impulse, const Vector2& point);
    void ApplyTorque(float torque);
    
    // Simulation
    void Update(float deltaTime);
    void ClearForces();
    
    // States
    bool IsStatic() const { return m_isStatic; }
    void SetStatic(bool isStatic);
    
    bool IsSleeping() const { return m_isSleeping; }
    void SetSleeping(bool sleeping) { m_isSleeping = sleeping; }

private:
    // Transform
    Vector2 m_position;
    float m_rotation;
    
    // Linear motion
    Vector2 m_velocity;
    Vector2 m_force;
    
    // Angular motion
    float m_angularVelocity;
    float m_torque;
    
    // Physical properties
    float m_mass;
    float m_invMass; // 1/mass for efficiency
    float m_inertia;
    float m_invInertia; // 1/inertia for efficiency
    
    // Material properties
    float m_restitution; // Bounciness
    float m_friction;
    
    // State flags
    bool m_isStatic;
    bool m_isSleeping;
    
    // Sleep threshold
    static constexpr float SLEEP_VELOCITY_THRESHOLD = 0.01f;
    static constexpr float SLEEP_ANGULAR_VELOCITY_THRESHOLD = 0.01f;
    static constexpr float SLEEP_TIME_THRESHOLD = 1.0f; // seconds
    
    float m_sleepTimer;
    
    void UpdateSleep(float deltaTime);
    void RecalculateInverseMass();
};

} // namespace BGE