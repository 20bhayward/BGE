#pragma once

// This file re-exports the core components from the existing Components.h
// and adds proper component registration support

#include "../../Components.h"
#include "../ComponentRegistry.h"
#include <iostream>

namespace BGE {

// Register all core components
inline void RegisterCoreComponents() {
    auto& registry = ComponentRegistry::Instance();
    
    std::cout << "RegisterCoreComponents: Starting component registration..." << std::endl;
    
    // Transform and hierarchy
    auto transformId = registry.RegisterComponent<TransformComponent>("TransformComponent");
    std::cout << "RegisterCoreComponents: TransformComponent registered with ID: " << transformId << std::endl;
    
    auto nameId = registry.RegisterComponent<NameComponent>("NameComponent");
    std::cout << "RegisterCoreComponents: NameComponent registered with ID: " << nameId << std::endl;
    
    // Physics and movement
    registry.RegisterComponent<VelocityComponent>("VelocityComponent");
    registry.RegisterComponent<RigidbodyComponent>("RigidbodyComponent");
    
    // Rendering
    registry.RegisterComponent<SpriteComponent>("SpriteComponent");
    registry.RegisterComponent<LightComponent>("LightComponent");
    
    // Gameplay
    registry.RegisterComponent<HealthComponent>("HealthComponent");
    registry.RegisterComponent<MaterialComponent>("MaterialComponent");
    
    std::cout << "RegisterCoreComponents: Total components registered: " << registry.GetComponentCount() << std::endl;
}

} // namespace BGE