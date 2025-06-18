#pragma once

// This file re-exports the core components from the existing Components.h
// and adds proper component registration support

#include "../../Components.h"
#include "../ComponentRegistry.h"

namespace BGE {

// Register all core components
inline void RegisterCoreComponents() {
    auto& registry = ComponentRegistry::Instance();
    
    // Transform and hierarchy
    registry.RegisterComponent<TransformComponent>("TransformComponent");
    registry.RegisterComponent<NameComponent>("NameComponent");
    
    // Physics and movement
    registry.RegisterComponent<VelocityComponent>("VelocityComponent");
    registry.RegisterComponent<RigidbodyComponent>("RigidbodyComponent");
    
    // Rendering
    registry.RegisterComponent<SpriteComponent>("SpriteComponent");
    registry.RegisterComponent<LightComponent>("LightComponent");
    
    // Gameplay
    registry.RegisterComponent<HealthComponent>("HealthComponent");
    registry.RegisterComponent<MaterialComponent>("MaterialComponent");
}

} // namespace BGE