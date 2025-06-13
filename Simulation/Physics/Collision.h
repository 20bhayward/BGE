#pragma once

#include "../../Core/Math/Vector2.h"

namespace BGE {

class RigidBody;

struct CollisionInfo {
    RigidBody* bodyA;
    RigidBody* bodyB;
    Vector2 contactPoint;
    Vector2 normal;
    float penetration;
    bool isColliding;
};

class CollisionDetector {
public:
    static bool CheckCollision(RigidBody* bodyA, RigidBody* bodyB, CollisionInfo& info);
    
    // Primitive collision tests
    static bool CircleVsCircle(const Vector2& centerA, float radiusA,
                              const Vector2& centerB, float radiusB,
                              CollisionInfo& info);
    
    static bool AABBvsAABB(const Vector2& minA, const Vector2& maxA,
                          const Vector2& minB, const Vector2& maxB,
                          CollisionInfo& info);
};

class CollisionResolver {
public:
    static void ResolveCollision(const CollisionInfo& info);
    static void ResolveCollisionWithFriction(const CollisionInfo& info);
    
private:
    static void ApplyImpulse(RigidBody* bodyA, RigidBody* bodyB,
                           const Vector2& normal, float impulse);
};

} // namespace BGE