#pragma once

#include "Entity.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Matrix4.h"
#include <string>
#include <algorithm>
#include <vector>
#include <sstream>
#include <unordered_map>

namespace BGE {

// Simple key-value serialization (we'll enhance this later with proper JSON)
using SerializationData = std::unordered_map<std::string, std::string>;

// Base serializable component interface
class ISerializableComponent {
public:
    virtual ~ISerializableComponent() = default;
    virtual SerializationData Serialize() const = 0;
    virtual void Deserialize(const SerializationData& data) = 0;
    virtual std::string GetTypeName() const = 0;
};

class TransformComponent : public Component, public ISerializableComponent {
public:
    Vector3 position{0.0f, 0.0f, 0.0f};
    float rotation = 0.0f;  // 2D rotation in radians
    Vector3 scale{1.0f, 1.0f, 1.0f};
    
    // Hierarchy support
    EntityID parent = INVALID_ENTITY_ID;
    std::vector<EntityID> children;
    
    // Cached world transform
    Matrix4 worldTransform = Matrix4::CreateIdentity();
    
    TransformComponent() = default;
    TransformComponent(const Vector3& pos, float rot = 0.0f, const Vector3& scl = {1, 1, 1})
        : position(pos), rotation(rot), scale(scl) {}
    
    // Serialization
    SerializationData Serialize() const override;
    void Deserialize(const SerializationData& data) override;
    std::string GetTypeName() const override { return "TransformComponent"; }
    
    // Hierarchy helpers
    void AddChild(EntityID child);
    void RemoveChild(EntityID child);
    void SetParent(EntityID parent);
};

class NameComponent : public Component {
public:
    std::string name;
    
    NameComponent() = default;
    NameComponent(const std::string& n) : name(n) {}
};

class SpriteComponent : public Component {
public:
    std::string texturePath;
    Vector2 size{1.0f, 1.0f};
    Vector2 uvOffset{0.0f, 0.0f};
    Vector2 uvScale{1.0f, 1.0f};
    bool visible = true;
    
    SpriteComponent() = default;
    SpriteComponent(const std::string& texture) : texturePath(texture) {}
};

class VelocityComponent : public Component {
public:
    Vector3 velocity{0.0f, 0.0f, 0.0f};
    Vector3 acceleration{0.0f, 0.0f, 0.0f};
    float damping = 0.99f;
    
    VelocityComponent() = default;
    VelocityComponent(const Vector3& vel) : velocity(vel) {}
};

class HealthComponent : public Component {
public:
    float maxHealth = 100.0f;
    float currentHealth = 100.0f;
    bool invulnerable = false;
    
    HealthComponent() = default;
    HealthComponent(float health) : maxHealth(health), currentHealth(health) {}
    
    float GetHealthPercentage() const { 
        return maxHealth > 0 ? currentHealth / maxHealth : 0.0f; 
    }
    
    bool IsAlive() const { 
        return currentHealth > 0.0f; 
    }
    
    void TakeDamage(float damage) {
        if (!invulnerable) {
            currentHealth = std::max(0.0f, currentHealth - damage);
        }
    }
    
    void Heal(float amount) {
        currentHealth = std::min(maxHealth, currentHealth + amount);
    }
};

class MaterialComponent : public Component {
public:
    uint32_t materialID = 0;
    float temperature = 20.0f; // Celsius
    float density = 1.0f;
    float hardness = 1.0f;
    
    MaterialComponent() = default;
    MaterialComponent(uint32_t id) : materialID(id) {}
};

// Light component for various light types
class LightComponent : public Component {
public:
    enum Type {
        Directional = 0,
        Point = 1,
        Spot = 2
    };
    
    Type type = Point;
    Vector3 color{1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;
    float range = 10.0f;  // For point/spot lights
    float innerCone = 30.0f;  // For spot lights (degrees)
    float outerCone = 45.0f;  // For spot lights (degrees)
    bool enabled = true;
    
    LightComponent() = default;
    LightComponent(Type lightType) : type(lightType) {}
    LightComponent(Type lightType, const Vector3& lightColor, float lightIntensity) 
        : type(lightType), color(lightColor), intensity(lightIntensity) {}
};

// Basic rigidbody component for physics simulation
class RigidbodyComponent : public Component {
public:
    float mass = 1.0f;
    Vector3 velocity{0.0f, 0.0f, 0.0f};
    Vector3 angularVelocity{0.0f, 0.0f, 0.0f};
    float drag = 0.0f;
    float angularDrag = 0.05f;
    bool useGravity = true;
    bool isKinematic = false;
    
    RigidbodyComponent() = default;
    RigidbodyComponent(float bodyMass) : mass(bodyMass) {}
};

} // namespace BGE