#include "Collision.h"
#include "RigidBody.h"
#include "../../Core/Math/Math.h"

namespace BGE {

bool CollisionDetector::CheckCollision(RigidBody* bodyA, RigidBody* bodyB, CollisionInfo& info) {
    // Simplified collision detection - treat all bodies as circles for now
    float radiusA = 1.0f; // TODO: Get actual radius from shape
    float radiusB = 1.0f;
    
    return CircleVsCircle(bodyA->GetPosition(), radiusA,
                         bodyB->GetPosition(), radiusB, info);
}

bool CollisionDetector::CircleVsCircle(const Vector2& centerA, float radiusA,
                                      const Vector2& centerB, float radiusB,
                                      CollisionInfo& info) {
    Vector2 direction = centerB - centerA;
    float distance = direction.Length();
    float radiusSum = radiusA + radiusB;
    
    if (distance < radiusSum) {
        info.isColliding = true;
        info.penetration = radiusSum - distance;
        
        if (distance > 0) {
            info.normal = direction / distance;
        } else {
            info.normal = Vector2(1.0f, 0.0f); // Default normal
        }
        
        info.contactPoint = centerA + info.normal * radiusA;
        return true;
    }
    
    info.isColliding = false;
    return false;
}

bool CollisionDetector::AABBvsAABB(const Vector2& minA, const Vector2& maxA,
                                  const Vector2& minB, const Vector2& maxB,
                                  CollisionInfo& info) {
    if (maxA.x < minB.x || minA.x > maxB.x ||
        maxA.y < minB.y || minA.y > maxB.y) {
        info.isColliding = false;
        return false;
    }
    
    info.isColliding = true;
    
    // Calculate overlap
    float overlapX = std::min(maxA.x, maxB.x) - std::max(minA.x, minB.x);
    float overlapY = std::min(maxA.y, maxB.y) - std::max(minA.y, minB.y);
    
    // Choose the axis with minimum overlap as separation axis
    if (overlapX < overlapY) {
        info.penetration = overlapX;
        info.normal = Vector2((maxA.x + minA.x) < (maxB.x + minB.x) ? -1.0f : 1.0f, 0.0f);
    } else {
        info.penetration = overlapY;
        info.normal = Vector2(0.0f, (maxA.y + minA.y) < (maxB.y + minB.y) ? -1.0f : 1.0f);
    }
    
    return true;
}

void CollisionResolver::ResolveCollision(const CollisionInfo& info) {
    if (!info.isColliding) return;
    
    RigidBody* bodyA = info.bodyA;
    RigidBody* bodyB = info.bodyB;
    
    // Calculate relative velocity
    Vector2 relativeVelocity = bodyB->GetVelocity() - bodyA->GetVelocity();
    float velocityAlongNormal = relativeVelocity.Dot(info.normal);
    
    // Don't resolve if velocities are separating
    if (velocityAlongNormal > 0) return;
    
    // Calculate restitution
    float restitution = std::min(bodyA->GetRestitution(), bodyB->GetRestitution());
    
    // Calculate impulse scalar
    float impulseScalar = -(1.0f + restitution) * velocityAlongNormal;
    
    float massA = bodyA->GetMass();
    float massB = bodyB->GetMass();
    
    if (!bodyA->IsStatic() && !bodyB->IsStatic()) {
        impulseScalar /= (1.0f / massA + 1.0f / massB);
    } else if (bodyA->IsStatic()) {
        impulseScalar /= (1.0f / massB);
    } else if (bodyB->IsStatic()) {
        impulseScalar /= (1.0f / massA);
    }
    
    ApplyImpulse(bodyA, bodyB, info.normal, impulseScalar);
    
    // Position correction to prevent sinking
    const float correctionPercent = 0.8f;
    const float slop = 0.01f;
    
    if (info.penetration > slop) {
        Vector2 correction = info.normal * (correctionPercent * (info.penetration - slop));
        
        if (!bodyA->IsStatic() && !bodyB->IsStatic()) {
            float totalMass = massA + massB;
            Vector2 correctionA = correction * (massB / totalMass);
            Vector2 correctionB = correction * (massA / totalMass);
            
            bodyA->SetPosition(bodyA->GetPosition() - correctionA);
            bodyB->SetPosition(bodyB->GetPosition() + correctionB);
        } else if (bodyA->IsStatic()) {
            bodyB->SetPosition(bodyB->GetPosition() + correction);
        } else if (bodyB->IsStatic()) {
            bodyA->SetPosition(bodyA->GetPosition() - correction);
        }
    }
}

void CollisionResolver::ResolveCollisionWithFriction(const CollisionInfo& info) {
    ResolveCollision(info);
    
    // TODO: Implement friction resolution
}

void CollisionResolver::ApplyImpulse(RigidBody* bodyA, RigidBody* bodyB,
                                   const Vector2& normal, float impulse) {
    Vector2 impulseVector = normal * impulse;
    
    if (!bodyA->IsStatic()) {
        bodyA->ApplyImpulse(-impulseVector);
    }
    
    if (!bodyB->IsStatic()) {
        bodyB->ApplyImpulse(impulseVector);
    }
}

} // namespace BGE