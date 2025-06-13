#include "RigidBody.h"
#include "../../Core/Math/Math.h"
#include <algorithm>

namespace BGE {

RigidBody::RigidBody()
    : m_position(0.0f, 0.0f)
    , m_rotation(0.0f)
    , m_velocity(0.0f, 0.0f)
    , m_force(0.0f, 0.0f)
    , m_angularVelocity(0.0f)
    , m_torque(0.0f)
    , m_mass(1.0f)
    , m_invMass(1.0f)
    , m_inertia(1.0f)
    , m_invInertia(1.0f)
    , m_restitution(0.3f)
    , m_friction(0.5f)
    , m_isStatic(false)
    , m_isSleeping(false)
    , m_sleepTimer(0.0f) {
}

RigidBody::~RigidBody() = default;

void RigidBody::SetMass(float mass) {
    m_mass = std::max(mass, 0.001f); // Prevent zero mass
    RecalculateInverseMass();
}

void RigidBody::ApplyForce(const Vector2& force) {
    if (m_isStatic) return;
    m_force += force;
    m_isSleeping = false;
}

void RigidBody::ApplyForceAtPoint(const Vector2& force, const Vector2& point) {
    if (m_isStatic) return;
    
    ApplyForce(force);
    
    Vector2 r = point - m_position;
    float torque = r.x * force.y - r.y * force.x; // Cross product in 2D
    ApplyTorque(torque);
}

void RigidBody::ApplyImpulse(const Vector2& impulse) {
    if (m_isStatic) return;
    m_velocity += impulse * m_invMass;
    m_isSleeping = false;
}

void RigidBody::ApplyImpulseAtPoint(const Vector2& impulse, const Vector2& point) {
    if (m_isStatic) return;
    
    ApplyImpulse(impulse);
    
    Vector2 r = point - m_position;
    float angularImpulse = r.x * impulse.y - r.y * impulse.x; // Cross product in 2D
    m_angularVelocity += angularImpulse * m_invInertia;
    m_isSleeping = false;
}

void RigidBody::ApplyTorque(float torque) {
    if (m_isStatic) return;
    m_torque += torque;
    m_isSleeping = false;
}

void RigidBody::Update(float deltaTime) {
    if (m_isStatic || m_isSleeping) return;
    
    // Integrate linear motion
    Vector2 acceleration = m_force * m_invMass;
    m_velocity += acceleration * deltaTime;
    m_position += m_velocity * deltaTime;
    
    // Integrate angular motion
    float angularAcceleration = m_torque * m_invInertia;
    m_angularVelocity += angularAcceleration * deltaTime;
    m_rotation += m_angularVelocity * deltaTime;
    
    // Update sleep state
    UpdateSleep(deltaTime);
    
    // Clear forces for next frame
    ClearForces();
}

void RigidBody::ClearForces() {
    m_force = Vector2(0.0f, 0.0f);
    m_torque = 0.0f;
}

void RigidBody::SetStatic(bool isStatic) {
    m_isStatic = isStatic;
    if (isStatic) {
        m_velocity = Vector2(0.0f, 0.0f);
        m_angularVelocity = 0.0f;
        m_invMass = 0.0f;
        m_invInertia = 0.0f;
    } else {
        RecalculateInverseMass();
    }
}

void RigidBody::UpdateSleep(float deltaTime) {
    if (m_isStatic) return;
    
    float velocityMagnitude = m_velocity.Length();
    float angularVelocityMagnitude = std::abs(m_angularVelocity);
    
    if (velocityMagnitude < SLEEP_VELOCITY_THRESHOLD && 
        angularVelocityMagnitude < SLEEP_ANGULAR_VELOCITY_THRESHOLD) {
        
        m_sleepTimer += deltaTime;
        
        if (m_sleepTimer >= SLEEP_TIME_THRESHOLD) {
            m_isSleeping = true;
            m_velocity = Vector2(0.0f, 0.0f);
            m_angularVelocity = 0.0f;
        }
    } else {
        m_sleepTimer = 0.0f;
    }
}

void RigidBody::RecalculateInverseMass() {
    if (m_isStatic) {
        m_invMass = 0.0f;
        m_invInertia = 0.0f;
    } else {
        m_invMass = 1.0f / m_mass;
        m_invInertia = 1.0f / m_inertia;
    }
}

} // namespace BGE