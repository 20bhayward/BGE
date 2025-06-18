#pragma once

#include <string>
#include <vector>
#include "ECS/EntityID.h"
#include "AssetTypes.h"

namespace BGE {

struct EngineInitializedEvent {
    bool success;
    std::string message;
};

struct EngineShuttingDownEvent {
    std::string reason;
};

struct FrameStartEvent {
    float deltaTime;
    uint64_t frameCount;
};

struct FrameEndEvent {
    float deltaTime;
    uint64_t frameCount;
    float frameTime;
};

struct WindowResizeEvent {
    uint32_t width;
    uint32_t height;
};

// Entity selection events for editor UI synchronization
struct EntitySelectionChangedEvent {
    std::vector<EntityID> selectedEntities;
    EntityID primarySelection = INVALID_ENTITY; // The main selected entity for single operations
    
    EntitySelectionChangedEvent() = default;
    EntitySelectionChangedEvent(EntityID single) : primarySelection(single) {
        if (single != INVALID_ENTITY) {
            selectedEntities.push_back(single);
        }
    }
    EntitySelectionChangedEvent(const std::vector<EntityID>& entities) : selectedEntities(entities) {
        if (!entities.empty()) {
            primarySelection = entities[0];
        }
    }
};

struct ApplicationStateChangedEvent {
    enum State {
        Initializing,
        Running,
        Paused,
        Shutting_Down
    } state;
};

// AssetSelectionChangedEvent is now defined in AssetTypes.h

// Material hover event for material inspector tooltip
struct MaterialHoverEvent {
    uint32_t materialID = 0;
    std::string materialName;
    std::string materialType;
    std::vector<std::string> materialTags;
    bool isHovering = false;  // false when no longer hovering
    
    MaterialHoverEvent() = default;
    MaterialHoverEvent(uint32_t id, const std::string& name, const std::string& type, const std::vector<std::string>& tags, bool hovering)
        : materialID(id), materialName(name), materialType(type), materialTags(tags), isHovering(hovering) {}
};

} // namespace BGE