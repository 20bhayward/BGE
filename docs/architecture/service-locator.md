# Service Locator System

## Overview

The Service Locator pattern provides a centralized registry for engine services, enabling dependency injection and loose coupling between systems.

## Implementation

**Files**: `Core/ServiceLocator.h/.cpp`, `Core/Services.h/.cpp`

### Core ServiceLocator Class

```cpp
class ServiceLocator {
public:
    static ServiceLocator& Instance();
    
    template<typename T>
    void RegisterService(std::shared_ptr<T> service);
    
    template<typename T>
    std::shared_ptr<T> GetService();
    
    template<typename T>
    bool HasService();
    
    template<typename T>
    void UnregisterService();
    
    void Clear();
};
```

### Service Registration

Services are registered during engine initialization:

```cpp
void Engine::RegisterCoreServices() {
    auto& serviceLocator = ServiceLocator::Instance();
    
    // Create and register renderer
    auto renderer = std::make_shared<Renderer>();
    if (renderer->Initialize(m_window.get())) {
        serviceLocator.RegisterService<Renderer>(renderer);
    }
    
    // Create and register simulation world
    auto world = std::make_shared<SimulationWorld>(width, height);
    serviceLocator.RegisterService<SimulationWorld>(world);
    
    // Register other services...
}
```

### Service Access

Use the Services helper namespace for convenient access:

```cpp
// Direct access
auto renderer = ServiceLocator::Instance().GetService<Renderer>();

// Convenient helper (recommended)
auto renderer = Services::GetRenderer();
auto world = Services::GetWorld();
auto audio = Services::GetAudio();
```

## Registered Services

| Service | Type | Description |
|---------|------|-------------|
| Renderer | `Renderer` | Graphics rendering system |
| SimulationWorld | `SimulationWorld` | Physics and material simulation |
| InputManager | `InputManager` | Input handling and events |
| AudioSystem | `AudioSystem` | Audio playback and 3D sound |
| AssetManager | `AssetManager` | Asset loading and caching |
| UISystem | `UISystem` | User interface rendering |

## Usage Examples

### In Application Code

```cpp
class MyApplication : public Application {
public:
    bool Initialize() override {
        // Get required services
        m_renderer = Services::GetRenderer();
        m_world = Services::GetWorld();
        m_input = Services::GetInput();
        
        if (!m_renderer || !m_world || !m_input) {
            BGE_LOG_ERROR("App", "Failed to get required services");
            return false;
        }
        
        // Use services...
        return true;
    }
    
private:
    std::shared_ptr<Renderer> m_renderer;
    std::shared_ptr<SimulationWorld> m_world;
    std::shared_ptr<InputManager> m_input;
};
```

### In System Code

```cpp
class PhysicsSystem {
public:
    void Initialize() {
        // Get dependencies
        m_world = Services::GetWorld();
        m_renderer = Services::GetRenderer(); // For debug rendering
        
        // Subscribe to events
        EventBus::Instance().Subscribe<EntityCreatedEvent>(
            [this](const EntityCreatedEvent& event) {
                HandleNewEntity(event.entityID);
            });
    }
    
    void Update(float deltaTime) {
        if (!m_world) return;
        
        // Process physics...
        m_world->UpdatePhysics(deltaTime);
    }
    
private:
    std::shared_ptr<SimulationWorld> m_world;
    std::shared_ptr<Renderer> m_renderer;
};
```

## Advanced Usage

### Conditional Service Access

```cpp
void SomeSystem::Update() {
    // Check if service is available
    if (auto audio = Services::GetAudio()) {
        audio->PlaySound("footstep.wav");
    }
    
    // Or use HasService for checking
    if (ServiceLocator::Instance().HasService<DebugRenderer>()) {
        auto debugRenderer = Services::Get<DebugRenderer>();
        debugRenderer->DrawWireframe(boundingBox);
    }
}
```

### Service Replacement for Testing

```cpp
class MockRenderer : public Renderer {
    // Mock implementation for testing
};

void SetupTestEnvironment() {
    // Replace real renderer with mock
    auto mockRenderer = std::make_shared<MockRenderer>();
    ServiceLocator::Instance().RegisterService<Renderer>(mockRenderer);
}
```

### Custom Services

```cpp
// Register custom game-specific services
class QuestSystem {
public:
    void LoadQuests(const std::string& questFile);
    void CompleteQuest(QuestID id);
};

// In game initialization
auto questSystem = std::make_shared<QuestSystem>();
questSystem->LoadQuests("data/quests.json");
ServiceLocator::Instance().RegisterService<QuestSystem>(questSystem);

// Access from anywhere
auto quests = ServiceLocator::Instance().GetService<QuestSystem>();
quests->CompleteQuest(QUEST_FIRST_SWORD);
```

## Best Practices

### 1. Service Lifetime Management
- Services are typically created once during engine initialization
- Services should clean up resources in their destructors
- Use RAII for automatic resource management

### 2. Service Dependencies
- Services can depend on other services
- Initialize services in dependency order
- Use lazy initialization when possible

```cpp
class AudioSystem {
public:
    bool Initialize() {
        // Get required services
        m_assetManager = Services::GetAssets();
        if (!m_assetManager) {
            BGE_LOG_ERROR("Audio", "AssetManager service required");
            return false;
        }
        
        // Initialize audio subsystem...
        return true;
    }
};
```

### 3. Error Handling
- Always check if services are available before use
- Services may be nullptr if not initialized or registration failed
- Use logging to track service availability issues

### 4. Thread Safety
- The current implementation is not thread-safe
- Services should be registered on the main thread during initialization
- Access from multiple threads requires synchronization

## Implementation Details

### Template Type Safety
The ServiceLocator uses `std::type_index` to ensure type safety:

```cpp
template<typename T>
void RegisterService(std::shared_ptr<T> service) {
    auto typeId = std::type_index(typeid(T));
    m_services[typeId] = service;
}

template<typename T>
std::shared_ptr<T> GetService() {
    auto typeId = std::type_index(typeid(T));
    auto it = m_services.find(typeId);
    if (it != m_services.end()) {
        return std::static_pointer_cast<T>(it->second);
    }
    return nullptr;
}
```

### Memory Management
- Services are stored as `std::shared_ptr<void>` to allow any type
- Type-safe casting is performed on retrieval
- Services keep shared ownership, preventing premature destruction

## Future Enhancements

### Planned Features
1. **Service Factories**: Support for lazy service creation
2. **Service Scopes**: Different lifetime scopes (singleton, per-frame, etc.)
3. **Interface-Based Services**: Register services by interface type
4. **Service Dependencies**: Automatic dependency resolution
5. **Thread Safety**: Concurrent access support

### Example Future API
```cpp
// Interface-based registration
ServiceLocator::RegisterInterface<IRenderer, VulkanRenderer>();

// Factory-based creation
ServiceLocator::RegisterFactory<AudioSystem>([](){ 
    return std::make_shared<AudioSystem>("audio_config.ini"); 
});

// Dependency injection
class PhysicsSystem {
public:
    PhysicsSystem(std::shared_ptr<IRenderer> renderer, 
                  std::shared_ptr<SimulationWorld> world)
        : m_renderer(renderer), m_world(world) {}
};

ServiceLocator::RegisterWithDependencies<PhysicsSystem>();
```