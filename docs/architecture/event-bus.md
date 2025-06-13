# Event Bus System

## Overview

The Event Bus provides a decoupled messaging system that allows components and systems to communicate without direct dependencies. This pattern promotes loose coupling and makes the system more maintainable and testable.

## Implementation

**Files**: `Core/EventBus.h/.cpp`, `Core/Events.h`

### Core EventBus Class

```cpp
class EventBus {
public:
    static EventBus& Instance();
    
    template<typename EventType>
    using EventHandler = std::function<void(const EventType&)>;
    
    template<typename EventType>
    void Subscribe(EventHandler<EventType> handler);
    
    template<typename EventType>
    void Publish(const EventType& event);
    
    void Clear();
};
```

### Event Definition

Events are simple data structures:

```cpp
// Core engine events (in Events.h)
struct EngineInitializedEvent {
    bool success;
    std::string message;
};

struct FrameStartEvent {
    float deltaTime;
    uint64_t frameCount;
};

struct WindowResizeEvent {
    uint32_t width;
    uint32_t height;
};

// Custom game events
struct PlayerDeathEvent {
    EntityID playerID;
    Vector3 deathPosition;
    std::string causeOfDeath;
};

struct MaterialChangedEvent {
    uint32_t x, y;
    MaterialID oldMaterial;
    MaterialID newMaterial;
    float temperature;
};
```

## Usage Examples

### Publishing Events

```cpp
// Engine publishes core events
EventBus::Instance().Publish(EngineInitializedEvent{true, "Engine started successfully"});
EventBus::Instance().Publish(FrameStartEvent{deltaTime, frameCount});

// Game systems publish domain events
EventBus::Instance().Publish(PlayerDeathEvent{
    playerID, 
    player->GetPosition(), 
    "Burned by lava"
});

EventBus::Instance().Publish(MaterialChangedEvent{
    x, y, 
    MATERIAL_WATER, 
    MATERIAL_STEAM, 
    100.0f
});
```

### Subscribing to Events

```cpp
class HealthSystem {
public:
    void Initialize() {
        // Subscribe to damage events
        EventBus::Instance().Subscribe<DamageEvent>([this](const DamageEvent& event) {
            ApplyDamage(event.targetEntity, event.damage, event.damageType);
        });
        
        // Subscribe to healing events
        EventBus::Instance().Subscribe<HealEvent>([this](const HealEvent& event) {
            ApplyHealing(event.targetEntity, event.healAmount);
        });
    }
    
private:
    void ApplyDamage(EntityID entity, float damage, DamageType type) {
        auto healthComp = EntityManager::Instance()
            .GetEntity(entity)->GetComponent<HealthComponent>();
            
        if (healthComp) {
            healthComp->TakeDamage(damage);
            
            if (!healthComp->IsAlive()) {
                EventBus::Instance().Publish(EntityDeathEvent{entity});
            }
        }
    }
};
```

### Application-Level Event Handling

```cpp
class InteractiveEditorApp : public Application {
public:
    bool Initialize() override {
        SubscribeToEvents();
        return true;
    }
    
private:
    void SubscribeToEvents() {
        auto& eventBus = EventBus::Instance();
        
        // Engine events
        eventBus.Subscribe<EngineInitializedEvent>([this](const EngineInitializedEvent& event) {
            BGE_LOG_INFO("Editor", "Engine initialized: " + event.message);
        });
        
        // Performance monitoring
        eventBus.Subscribe<FrameStartEvent>([this](const FrameStartEvent& event) {
            if (event.frameCount % 60 == 0) { // Every second at 60 FPS
                m_avgFrameTime = CalculateAverageFrameTime();
            }
        });
        
        // Window events
        eventBus.Subscribe<WindowResizeEvent>([this](const WindowResizeEvent& event) {
            BGE_LOG_INFO("Editor", "Window resized to " + 
                        std::to_string(event.width) + "x" + std::to_string(event.height));
            UpdateViewport(event.width, event.height);
        });
    }
};
```

## Core Engine Events

### System Events

| Event | When Fired | Data |
|-------|------------|------|
| `EngineInitializedEvent` | Engine initialization complete | success, message |
| `EngineShuttingDownEvent` | Engine shutdown started | reason |
| `ApplicationStateChangedEvent` | App state changes | new state |

### Frame Events

| Event | When Fired | Data |
|-------|------------|------|
| `FrameStartEvent` | Beginning of frame | deltaTime, frameCount |
| `FrameEndEvent` | End of frame | deltaTime, frameCount, frameTime |

### Window Events

| Event | When Fired | Data |
|-------|------------|------|
| `WindowResizeEvent` | Window size changed | width, height |
| `WindowCloseEvent` | Window close requested | - |
| `WindowFocusEvent` | Window focus changed | hasFocus |

## Custom Event Types

### Game Events

```cpp
// Combat events
struct DamageEvent {
    EntityID attacker;
    EntityID target;
    float damage;
    DamageType type;
    Vector3 hitPosition;
};

struct HealEvent {
    EntityID healer;
    EntityID target;
    float healAmount;
    HealType type;
};

// Crafting events
struct ItemCraftedEvent {
    EntityID crafter;
    ItemID craftedItem;
    std::vector<ItemID> materials;
    float craftingTime;
    CraftingQuality quality;
};

struct ForgeTemperatureChangedEvent {
    EntityID forgeID;
    float oldTemperature;
    float newTemperature;
    bool isOptimal;
};

// World events
struct MaterialReactionEvent {
    uint32_t x, y;
    MaterialID reactant1;
    MaterialID reactant2;
    MaterialID product;
    float energyReleased;
};

struct ChunkLoadedEvent {
    int32_t chunkX, chunkY;
    uint32_t entityCount;
    float loadTime;
};
```

### UI Events

```cpp
struct MenuOpenedEvent {
    std::string menuName;
    MenuType type;
};

struct ButtonClickedEvent {
    std::string buttonID;
    Vector2 clickPosition;
};

struct InventoryChangedEvent {
    EntityID entity;
    ItemID item;
    int32_t quantity;
    InventoryOperation operation; // ADD, REMOVE, MOVE
};
```

## Event Processing Patterns

### Immediate Processing
Default behavior - events are processed immediately when published:

```cpp
EventBus::Instance().Publish(DamageEvent{...}); // Handlers called immediately
```

### Deferred Processing (Future Enhancement)
Queue events for processing at specific times:

```cpp
// Planned future API
EventBus::Instance().PublishDeferred(DamageEvent{...});
EventBus::Instance().ProcessDeferredEvents(); // Called at end of frame
```

### Event Filtering
Use lambdas for conditional event handling:

```cpp
// Only handle damage to player entities
EventBus::Instance().Subscribe<DamageEvent>([this](const DamageEvent& event) {
    auto entity = EntityManager::Instance().GetEntity(event.target);
    if (entity && entity->HasComponent<PlayerComponent>()) {
        HandlePlayerDamage(event);
    }
});
```

### Event Chains
Events can trigger other events:

```cpp
EventBus::Instance().Subscribe<EntityDeathEvent>([](const EntityDeathEvent& event) {
    // Death triggers loot drop
    EventBus::Instance().Publish(LootDropEvent{
        event.entityID,
        event.position,
        CalculateLoot(event.entityID)
    });
    
    // Death triggers sound effect
    EventBus::Instance().Publish(AudioEvent{
        "death_sound.wav",
        event.position,
        1.0f // volume
    });
});
```

## Best Practices

### 1. Event Design
- Keep events as simple data structures
- Include all necessary context in the event
- Use descriptive names that indicate when the event occurs

```cpp
// Good: Clear when this happens
struct PlayerLeveledUpEvent {
    EntityID playerID;
    uint32_t oldLevel;
    uint32_t newLevel;
    uint32_t skillPointsGained;
};

// Avoid: Unclear timing
struct PlayerLevelEvent {
    EntityID playerID;
    uint32_t level; // Before or after leveling?
};
```

### 2. Handler Management
- Subscribe in Initialize() methods
- No need to unsubscribe (handlers are cleared on shutdown)
- Use weak references if handler objects might be destroyed

```cpp
class WeaponSystem {
public:
    void Initialize() {
        // Good: Subscribe in initialization
        EventBus::Instance().Subscribe<AttackEvent>([this](const AttackEvent& event) {
            ProcessAttack(event);
        });
    }
    
    // Handlers automatically cleaned up when EventBus is cleared
};
```

### 3. Performance Considerations
- Events are processed synchronously
- Keep event handlers lightweight
- Use events for coordination, not heavy computation

```cpp
// Good: Lightweight coordination
EventBus::Instance().Subscribe<PlayerMovedEvent>([](const PlayerMovedEvent& event) {
    AudioSystem::UpdateListenerPosition(event.newPosition);
    CameraSystem::FollowTarget(event.playerID);
});

// Avoid: Heavy computation in handlers
EventBus::Instance().Subscribe<PlayerMovedEvent>([](const PlayerMovedEvent& event) {
    RegenerateEntireWorld(); // Too expensive!
});
```

### 4. Error Handling
- Event handlers should not throw exceptions
- Use logging for error reporting in handlers

```cpp
EventBus::Instance().Subscribe<SaveGameEvent>([](const SaveGameEvent& event) {
    try {
        SaveSystem::SaveGame(event.saveSlot);
        BGE_LOG_INFO("Save", "Game saved successfully");
    } catch (const std::exception& e) {
        BGE_LOG_ERROR("Save", "Failed to save game: " + std::string(e.what()));
        // Don't rethrow - would crash other handlers
    }
});
```

## Implementation Details

### Type-Safe Event Handling
The EventBus uses `std::type_index` for type safety:

```cpp
template<typename EventType>
void Subscribe(EventHandler<EventType> handler) {
    auto typeId = std::type_index(typeid(EventType));
    auto& handlers = m_handlers[typeId];
    handlers.push_back(std::make_shared<HandlerWrapper<EventType>>(handler));
}
```

### Handler Storage
Event handlers are stored as type-erased wrappers:

```cpp
class HandlerWrapperBase {
public:
    virtual ~HandlerWrapperBase() = default;
    virtual void Call(const void* event) = 0;
};

template<typename EventType>
class HandlerWrapper : public HandlerWrapperBase {
public:
    HandlerWrapper(EventHandler<EventType> handler) : m_handler(handler) {}
    
    void Call(const void* event) override {
        m_handler(*static_cast<const EventType*>(event));
    }
    
private:
    EventHandler<EventType> m_handler;
};
```

## Future Enhancements

### Planned Features
1. **Event Priorities**: Control handler execution order
2. **Event Filtering**: Built-in filtering mechanisms  
3. **Deferred Events**: Queue events for later processing
4. **Event History**: Track recent events for debugging
5. **Performance Monitoring**: Event processing metrics

### Example Future API
```cpp
// Priority-based handlers
EventBus::Subscribe<DamageEvent>(handler, EventPriority::HIGH);

// Event filtering
EventBus::Subscribe<DamageEvent>(handler, [](const DamageEvent& e) {
    return e.target == playerEntityID; // Only player damage
});

// Deferred processing
EventBus::PublishDeferred<ExplosionEvent>({position, radius, damage});
EventBus::ProcessDeferredEvents(EventCategory::PHYSICS);
```