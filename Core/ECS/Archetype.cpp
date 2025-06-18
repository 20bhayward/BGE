#include "Archetype.h"
#include "ComponentRegistry.h"
#include "../Components.h"

namespace BGE {

std::unique_ptr<IComponentStorage> Archetype::CreateStorageForType(std::type_index typeIndex) {
    // Use the ComponentRegistry to create storage dynamically
    // This is a simplified version that works with the registry system
    
    auto& registry = ComponentRegistry::Instance();
    
    // Find the component type ID
    ComponentTypeID typeID = INVALID_COMPONENT_TYPE;
    for (const auto& [id, info] : registry.GetAllComponents()) {
        if (info.typeIndex == typeIndex) {
            typeID = id;
            break;
        }
    }
    
    if (typeID == INVALID_COMPONENT_TYPE) {
        return nullptr;
    }
    
    const ComponentInfo* info = registry.GetComponentInfo(typeID);
    if (!info) {
        return nullptr;
    }
    
    // Create a generic storage that uses the registry's function pointers
    // For now, we'll use a simple implementation
    // In a complete implementation, this would be more sophisticated
    
    // Try known types first for compatibility
    if (typeIndex == std::type_index(typeid(TransformComponent))) {
        return std::make_unique<TypedComponentStorage<TransformComponent>>();
    }
    else if (typeIndex == std::type_index(typeid(VelocityComponent))) {
        return std::make_unique<TypedComponentStorage<VelocityComponent>>();
    }
    else if (typeIndex == std::type_index(typeid(HealthComponent))) {
        return std::make_unique<TypedComponentStorage<HealthComponent>>();
    }
    else if (typeIndex == std::type_index(typeid(MaterialComponent))) {
        return std::make_unique<TypedComponentStorage<MaterialComponent>>();
    }
    else if (typeIndex == std::type_index(typeid(SpriteComponent))) {
        return std::make_unique<TypedComponentStorage<SpriteComponent>>();
    }
    else if (typeIndex == std::type_index(typeid(LightComponent))) {
        return std::make_unique<TypedComponentStorage<LightComponent>>();
    }
    else if (typeIndex == std::type_index(typeid(RigidbodyComponent))) {
        return std::make_unique<TypedComponentStorage<RigidbodyComponent>>();
    }
    else if (typeIndex == std::type_index(typeid(NameComponent))) {
        return std::make_unique<TypedComponentStorage<NameComponent>>();
    }
    
    // For unknown types, we need a way to create storage dynamically
    // This is a limitation of the current implementation
    return nullptr;
}

} // namespace BGE