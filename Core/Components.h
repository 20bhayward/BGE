#pragma once

#include "ECS/EntityID.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Matrix4.h"
#include "Math/Quaternion.h"
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

class TransformComponent : public ISerializableComponent {
public:
    Vector3 position{0.0f, 0.0f, 0.0f};
    float rotation = 0.0f;  // 2D rotation in radians (legacy, will be deprecated)
    Quaternion rotation3D;  // 3D rotation using quaternions
    Vector3 scale{1.0f, 1.0f, 1.0f};
    
    // Hierarchy support
    EntityID parent = INVALID_ENTITY;
    std::vector<EntityID> children;
    
    // Cached world transform
    Matrix4 worldTransform = Matrix4::CreateIdentity();
    
    TransformComponent() = default;
    TransformComponent(const Vector3& pos, float rot = 0.0f, const Vector3& scl = {1, 1, 1})
        : position(pos), rotation(rot), scale(scl) {
        // Initialize 3D rotation from 2D rotation
        rotation3D = Quaternion::FromEuler(0.0f, 0.0f, rot);
    }
    
    TransformComponent(const Vector3& pos, const Quaternion& rot, const Vector3& scl = {1, 1, 1})
        : position(pos), rotation3D(rot), scale(scl) {
        // Update 2D rotation from quaternion (for backward compatibility)
        rotation = rotation3D.ToEuler().z;
    }
    
    // Serialization
    SerializationData Serialize() const override;
    void Deserialize(const SerializationData& data) override;
    std::string GetTypeName() const override { return "TransformComponent"; }
    
    // Rotation helpers
    void SetRotation2D(float radians) {
        rotation = radians;
        rotation3D = Quaternion::FromEuler(0.0f, 0.0f, radians);
    }
    
    void SetRotation3D(const Quaternion& rot) {
        rotation3D = rot;
        rotation = rot.ToEuler().z; // Update 2D rotation for compatibility
    }
    
    void SetEulerAngles(float pitch, float yaw, float roll) {
        rotation3D = Quaternion::FromEuler(pitch, yaw, roll);
        rotation = roll; // Update 2D rotation for compatibility
    }
    
    Vector3 GetEulerAngles() const {
        return rotation3D.ToEuler();
    }
    
    // Compute local transform matrix
    Matrix4 GetLocalTransform() const {
        return Matrix4::TRS(position, rotation3D, scale);
    }
    
    // Hierarchy helpers
    void AddChild(EntityID child);
    void RemoveChild(EntityID child);
    void SetParent(EntityID parent);
};

class NameComponent {
public:
    std::string name;
    
    NameComponent() = default;
    NameComponent(const std::string& n) : name(n) {}
};

class SpriteComponent {
public:
    std::string texturePath;
    Vector2 size{1.0f, 1.0f};
    Vector2 uvOffset{0.0f, 0.0f};
    Vector2 uvScale{1.0f, 1.0f};
    bool visible = true;
    
    SpriteComponent() = default;
    SpriteComponent(const std::string& texture) : texturePath(texture) {}
};

class VelocityComponent : public ISerializableComponent {
public:
    Vector3 velocity{0.0f, 0.0f, 0.0f};      // Linear velocity
    Vector3 acceleration{0.0f, 0.0f, 0.0f};   // Linear acceleration
    Vector3 angular{0.0f, 0.0f, 0.0f};        // Angular velocity (rad/s)
    float damping = 0.99f;
    
    // Compatibility getter/setter
    Vector3& linear() { return velocity; }
    const Vector3& linear() const { return velocity; }
    
    VelocityComponent() = default;
    VelocityComponent(const Vector3& vel) : velocity(vel) {}
    VelocityComponent(const Vector3& vel, const Vector3& ang) : velocity(vel), angular(ang) {}
    
    // Serialization
    SerializationData Serialize() const override {
        return {
            {"velocity_x", std::to_string(velocity.x)},
            {"velocity_y", std::to_string(velocity.y)},
            {"velocity_z", std::to_string(velocity.z)},
            {"acceleration_x", std::to_string(acceleration.x)},
            {"acceleration_y", std::to_string(acceleration.y)},
            {"acceleration_z", std::to_string(acceleration.z)},
            {"angular_x", std::to_string(angular.x)},
            {"angular_y", std::to_string(angular.y)},
            {"angular_z", std::to_string(angular.z)},
            {"damping", std::to_string(damping)}
        };
    }
    
    void Deserialize(const SerializationData& data) override {
        if (auto it = data.find("velocity_x"); it != data.end()) velocity.x = std::stof(it->second);
        if (auto it = data.find("velocity_y"); it != data.end()) velocity.y = std::stof(it->second);
        if (auto it = data.find("velocity_z"); it != data.end()) velocity.z = std::stof(it->second);
        if (auto it = data.find("acceleration_x"); it != data.end()) acceleration.x = std::stof(it->second);
        if (auto it = data.find("acceleration_y"); it != data.end()) acceleration.y = std::stof(it->second);
        if (auto it = data.find("acceleration_z"); it != data.end()) acceleration.z = std::stof(it->second);
        if (auto it = data.find("angular_x"); it != data.end()) angular.x = std::stof(it->second);
        if (auto it = data.find("angular_y"); it != data.end()) angular.y = std::stof(it->second);
        if (auto it = data.find("angular_z"); it != data.end()) angular.z = std::stof(it->second);
        if (auto it = data.find("damping"); it != data.end()) damping = std::stof(it->second);
    }
    
    std::string GetTypeName() const override { return "VelocityComponent"; }
};

class HealthComponent {
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

class MaterialComponent {
public:
    uint32_t materialID = 0;
    float temperature = 20.0f; // Celsius
    float density = 1.0f;
    float hardness = 1.0f;
    
    MaterialComponent() = default;
    MaterialComponent(uint32_t id) : materialID(id) {}
};

// Light component for various light types
class LightComponent {
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
class RigidbodyComponent {
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