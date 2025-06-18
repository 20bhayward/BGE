#pragma once

#include "../../Math/Vector3.h"
#include "../EntityID.h"
#include <vector>
#include <string>
#include <unordered_map>

namespace BGE {

// Inventory component for item storage and management
struct InventoryComponent {
    struct ItemSlot {
        EntityID itemEntity = INVALID_ENTITY;
        uint32_t quantity = 0;
        uint32_t maxStack = 1;
    };
    
    std::vector<ItemSlot> slots;
    uint32_t maxSlots = 20;
    float maxWeight = 100.0f;
    float currentWeight = 0.0f;
    
    InventoryComponent() {
        slots.resize(maxSlots);
    }
    
    bool AddItem(EntityID item, uint32_t quantity = 1) {
        // Find empty slot or stackable item
        for (auto& slot : slots) {
            if (slot.itemEntity == INVALID_ENTITY) {
                slot.itemEntity = item;
                slot.quantity = quantity;
                return true;
            } else if (slot.itemEntity == item && slot.quantity < slot.maxStack) {
                uint32_t canAdd = std::min(quantity, slot.maxStack - slot.quantity);
                slot.quantity += canAdd;
                if (canAdd < quantity) {
                    return AddItem(item, quantity - canAdd);
                }
                return true;
            }
        }
        return false; // No space
    }
    
    bool RemoveItem(EntityID item, uint32_t quantity = 1) {
        for (auto& slot : slots) {
            if (slot.itemEntity == item) {
                if (slot.quantity >= quantity) {
                    slot.quantity -= quantity;
                    if (slot.quantity == 0) {
                        slot.itemEntity = INVALID_ENTITY;
                    }
                    return true;
                }
            }
        }
        return false;
    }
    
    uint32_t GetItemCount(EntityID item) const {
        uint32_t count = 0;
        for (const auto& slot : slots) {
            if (slot.itemEntity == item) {
                count += slot.quantity;
            }
        }
        return count;
    }
};

// AI component for behavior state and decision making
struct AIComponent {
    enum class State {
        Idle,
        Patrol,
        Chase,
        Attack,
        Flee,
        Dead
    };
    
    enum class Behavior {
        Passive,
        Defensive,
        Aggressive,
        Neutral
    };
    
    State currentState = State::Idle;
    State previousState = State::Idle;
    Behavior behavior = Behavior::Neutral;
    
    EntityID targetEntity = INVALID_ENTITY;
    Vector3 targetPosition;
    
    float aggressionRadius = 10.0f;
    float attackRadius = 2.0f;
    float fleeHealthThreshold = 0.2f;
    
    float stateTimer = 0.0f;
    float decisionInterval = 0.5f; // How often AI makes decisions
    float lastDecisionTime = 0.0f;
    
    // Patrol data
    std::vector<Vector3> patrolPoints;
    size_t currentPatrolIndex = 0;
    
    // Memory of recent interactions
    struct Memory {
        EntityID entity;
        float threatLevel;
        float lastSeenTime;
    };
    std::vector<Memory> memories;
    
    void ChangeState(State newState) {
        previousState = currentState;
        currentState = newState;
        stateTimer = 0.0f;
    }
    
    bool IsHostile() const {
        return behavior == Behavior::Aggressive || 
               (behavior == Behavior::Defensive && currentState == State::Attack);
    }
};

// Temperature component for thermal simulation
struct TemperatureComponent {
    float currentTemperature = 20.0f; // Celsius
    float targetTemperature = 20.0f;  // Equilibrium temperature
    float heatCapacity = 1000.0f;     // J/K
    float thermalConductivity = 0.5f; // W/(m·K)
    float mass = 1.0f;                // kg
    
    // Temperature limits
    float freezingPoint = 0.0f;
    float boilingPoint = 100.0f;
    float ignitionPoint = 300.0f;
    
    // State tracking
    bool isFrozen = false;
    bool isBoiling = false;
    bool isBurning = false;
    
    // Heat transfer
    float heatGainRate = 0.0f;  // W
    float heatLossRate = 0.0f;  // W
    
    void UpdateTemperature(float deltaTime) {
        float netHeatRate = heatGainRate - heatLossRate;
        float deltaTemp = (netHeatRate * deltaTime) / (mass * heatCapacity);
        currentTemperature += deltaTemp;
        
        // Update states
        isFrozen = currentTemperature <= freezingPoint;
        isBoiling = currentTemperature >= boilingPoint;
        isBurning = currentTemperature >= ignitionPoint;
    }
    
    float GetThermalEnergy() const {
        return mass * heatCapacity * currentTemperature;
    }
};

// Enhanced physics component with more properties
struct PhysicsComponent {
    Vector3 velocity{0.0f, 0.0f, 0.0f};
    Vector3 acceleration{0.0f, 0.0f, 0.0f};
    Vector3 angularVelocity{0.0f, 0.0f, 0.0f};
    
    float mass = 1.0f;
    float drag = 0.1f;
    float angularDrag = 0.1f;
    float restitution = 0.5f; // Bounciness
    float friction = 0.5f;
    
    bool useGravity = true;
    bool isKinematic = false;
    bool isTrigger = false;
    
    // Constraints
    bool freezePositionX = false;
    bool freezePositionY = false;
    bool freezePositionZ = false;
    bool freezeRotationX = false;
    bool freezeRotationY = false;
    bool freezeRotationZ = false;
    
    void ApplyForce(const Vector3& force) {
        if (!isKinematic && mass > 0) {
            acceleration += force / mass;
        }
    }
    
    void ApplyImpulse(const Vector3& impulse) {
        if (!isKinematic && mass > 0) {
            velocity += impulse / mass;
        }
    }
};

// Render component for visual representation
struct RenderComponent {
    enum class RenderType {
        Sprite,
        Mesh,
        ParticleSystem,
        Text
    };
    
    RenderType type = RenderType::Sprite;
    std::string resourcePath;
    
    bool visible = true;
    int renderLayer = 0;
    int sortingOrder = 0;
    
    // Material properties
    Vector3 color{1.0f, 1.0f, 1.0f};
    float alpha = 1.0f;
    
    // Sprite specific
    Vector2 spriteSize{1.0f, 1.0f};
    Vector2 spriteOffset{0.0f, 0.0f};
    
    // Animation
    bool animated = false;
    uint32_t currentFrame = 0;
    float animationSpeed = 1.0f;
};

// Animation component for skeletal and sprite animation
struct AnimationComponent {
    struct AnimationClip {
        std::string name;
        uint32_t startFrame;
        uint32_t endFrame;
        float duration;
        bool loop = true;
    };
    
    std::unordered_map<std::string, AnimationClip> clips;
    std::string currentClip;
    float currentTime = 0.0f;
    float playbackSpeed = 1.0f;
    bool isPlaying = false;
    
    void Play(const std::string& clipName) {
        auto it = clips.find(clipName);
        if (it != clips.end()) {
            currentClip = clipName;
            currentTime = 0.0f;
            isPlaying = true;
        }
    }
    
    void Stop() {
        isPlaying = false;
    }
    
    void Update(float deltaTime) {
        if (!isPlaying || currentClip.empty()) return;
        
        auto it = clips.find(currentClip);
        if (it == clips.end()) return;
        
        currentTime += deltaTime * playbackSpeed;
        
        if (currentTime >= it->second.duration) {
            if (it->second.loop) {
                currentTime = fmod(currentTime, it->second.duration);
            } else {
                currentTime = it->second.duration;
                isPlaying = false;
            }
        }
    }
};

// Tag component for entity categorization
struct TagComponent {
    std::vector<std::string> tags;
    
    void AddTag(const std::string& tag) {
        if (std::find(tags.begin(), tags.end(), tag) == tags.end()) {
            tags.push_back(tag);
        }
    }
    
    void RemoveTag(const std::string& tag) {
        tags.erase(std::remove(tags.begin(), tags.end(), tag), tags.end());
    }
    
    bool HasTag(const std::string& tag) const {
        return std::find(tags.begin(), tags.end(), tag) != tags.end();
    }
};

} // namespace BGE