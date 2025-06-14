#pragma once

#include <vector>
#include <string>
#include <typeindex>

namespace BGE {

// Forward declarations
class Entity;

// Component type identifier
using ComponentType = std::type_index;

// System update priority (lower values update first)
enum class SystemPriority : int {
    Input = 100,
    Physics = 200,
    Movement = 300,
    Animation = 400,
    AI = 500,
    Gameplay = 600,
    Rendering = 700,
    UI = 800,
    Debug = 900,
    Default = 500
};

// Base interface for all systems
class ISystem {
public:
    virtual ~ISystem() = default;
    
    // Core system lifecycle
    virtual void Initialize() = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Shutdown() = 0;
    
    // System requirements
    virtual std::vector<ComponentType> GetRequiredComponents() const = 0;
    virtual std::vector<ComponentType> GetOptionalComponents() const { return {}; }
    
    // System metadata
    virtual std::string GetName() const = 0;
    virtual SystemPriority GetPriority() const { return SystemPriority::Default; }
    
    // System state
    virtual bool IsEnabled() const { return m_enabled; }
    virtual void SetEnabled(bool enabled) { m_enabled = enabled; }
    
protected:
    bool m_enabled = true;
};

} // namespace BGE