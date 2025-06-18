#include "Archetype.h"
#include "ComponentRegistry.h"
#include "ComponentStorage.h"
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
    
    // Create generic storage using component registry metadata
    return std::make_unique<GenericComponentStorage>(*info);
}

} // namespace BGE