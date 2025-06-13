#pragma once

#include "Entity.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include <string>
#include <algorithm>

namespace BGE {

class TransformComponent : public Component {
public:
    Vector3 position{0.0f, 0.0f, 0.0f};
    Vector3 rotation{0.0f, 0.0f, 0.0f};
    Vector3 scale{1.0f, 1.0f, 1.0f};
    
    TransformComponent() = default;
    TransformComponent(const Vector3& pos, const Vector3& rot = {0, 0, 0}, const Vector3& scl = {1, 1, 1})
        : position(pos), rotation(rot), scale(scl) {}
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

} // namespace BGE