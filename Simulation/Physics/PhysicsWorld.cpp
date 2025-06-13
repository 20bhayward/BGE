#include "PhysicsWorld.h"

namespace BGE {

PhysicsWorld::PhysicsWorld() : m_gravity(0.0f, 9.81f) {
}

PhysicsWorld::~PhysicsWorld() = default;

void PhysicsWorld::Update(float deltaTime) {
    ApplyGravity(deltaTime);
    IntegrateForces(deltaTime);
    CheckCollisions();
    SolveConstraints();
    IntegrateVelocities(deltaTime);
}

RigidBody* PhysicsWorld::CreateRigidBody() {
    auto body = std::make_unique<RigidBody>();
    RigidBody* bodyPtr = body.get();
    m_bodies.push_back(std::move(body));
    return bodyPtr;
}

void PhysicsWorld::DestroyRigidBody(RigidBody* body) {
    auto it = std::find_if(m_bodies.begin(), m_bodies.end(),
        [body](const std::unique_ptr<RigidBody>& ptr) {
            return ptr.get() == body;
        });
    
    if (it != m_bodies.end()) {
        m_bodies.erase(it);
    }
}

void PhysicsWorld::CheckCollisions() {
    // TODO: Implement collision detection
}

void PhysicsWorld::ApplyGravity(float deltaTime) {
    (void)deltaTime;
    for (auto& body : m_bodies) {
        if (!body->IsStatic()) {
            Vector2 gravityForce = m_gravity * body->GetMass();
            body->ApplyForce(gravityForce);
        }
    }
}

void PhysicsWorld::IntegrateForces(float deltaTime) {
    (void)deltaTime;
    for (auto& body : m_bodies) {
        body->Update(deltaTime);
    }
}

void PhysicsWorld::SolveConstraints() {
    // TODO: Implement constraint solving
}

void PhysicsWorld::IntegrateVelocities(float deltaTime) {
    (void)deltaTime;
    // Already handled in RigidBody::Update
}

} // namespace BGE