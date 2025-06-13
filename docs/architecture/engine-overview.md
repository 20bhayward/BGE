# Engine Architecture Overview

## Current Architecture (Phase 1 Complete)

BGE has been refactored from a monolithic singleton-based architecture to a modular, service-based design that promotes loose coupling, testability, and extensibility.

## Core Architectural Patterns

### 1. Service Locator Pattern
**Location**: `Core/ServiceLocator.h/.cpp`

Replaces direct singleton dependencies with a registry-based approach:

```cpp
// Old approach (tightly coupled)
auto world = Engine::Instance().GetWorld();

// New approach (service-based)
auto world = Services::GetWorld();
```

**Benefits**:
- Systems can be tested in isolation
- Easy to mock dependencies for testing
- Runtime service swapping for different configurations

### 2. Event Bus System
**Location**: `Core/EventBus.h/.cpp`, `Core/Events.h`

Enables decoupled communication between systems:

```cpp
// Publishing events
EventBus::Instance().Publish(FrameStartEvent{deltaTime, frameCount});

// Subscribing to events
EventBus::Instance().Subscribe<WindowResizeEvent>([](const WindowResizeEvent& event) {
    // Handle window resize
});
```

**Benefits**:
- No direct dependencies between systems
- Easy to add new event types
- Centralized event management

### 3. Entity-Component System Foundation
**Location**: `Core/Entity.h/.cpp`, `Core/Components.h`

Unified game object model:

```cpp
auto entity = EntityManager::Instance().CreateEntity("Player");
entity->AddComponent<TransformComponent>(Vector3{0, 0, 0});
entity->AddComponent<HealthComponent>(100.0f);
entity->AddComponent<VelocityComponent>();
```

**Benefits**:
- Composition over inheritance
- Data-oriented design
- High performance through cache-friendly memory layout

## System Initialization Flow

```
1. Engine::Initialize()
   ├── Logger::Initialize()
   ├── ConfigManager::LoadFromFile()
   ├── InitializeServices()
   │   ├── Create Window
   │   └── RegisterCoreServices()
   │       ├── Renderer
   │       ├── SimulationWorld
   │       ├── InputManager
   │       ├── AudioSystem
   │       ├── AssetManager
   │       └── UISystem
   └── Publish EngineInitializedEvent
```

## Main Loop Architecture

```
Main Loop:
├── EventBus::Publish(FrameStartEvent)
├── Window::PollEvents()
├── InputManager::Update()
├── Application::Update()
├── SimulationWorld::Update()
├── AudioSystem::Update()
├── Renderer::BeginFrame()
├── Renderer::RenderWorld()
├── UISystem::BeginFrame()
├── Application::Render()
├── UISystem::EndFrame()
├── Renderer::EndFrame()
├── Window::SwapBuffers()
└── EventBus::Publish(FrameEndEvent)
```

## Configuration System
**Location**: `Core/ConfigManager.h/.cpp`

Data-driven engine behavior:

```ini
# config.ini
window.width = 1280
window.height = 720
renderer.vsync = true
log.level = DEBUG
simulation.update_frequency = 60.0
```

## Logging System
**Location**: `Core/Logger.h/.cpp`

Professional logging with categories and levels:

```cpp
BGE_LOG_INFO("Renderer", "Initializing Vulkan device");
BGE_LOG_ERROR("Physics", "Collision detection failed for entity " + entityID);
BGE_LOG_DEBUG("AI", "Pathfinding completed in " + std::to_string(time) + "ms");
```

## Directory Structure

```
BGE/
├── Core/                     # Core engine systems
│   ├── Engine.h/.cpp        # Main engine class
│   ├── ServiceLocator.*     # Service dependency injection
│   ├── EventBus.*          # Event messaging system
│   ├── Logger.*            # Logging system
│   ├── ConfigManager.*     # Configuration management
│   ├── Entity.*            # Entity-Component system
│   ├── Components.h        # Standard component types
│   ├── Math/               # Math utilities
│   ├── Memory/             # Memory management
│   ├── Threading/          # Job system and threading
│   ├── Input/              # Input management
│   └── UI/                 # UI systems
├── Renderer/               # Rendering systems
├── Simulation/             # Physics and simulation
├── Audio/                  # Audio systems
├── AssetPipeline/         # Asset loading and management
├── Examples/              # Example applications
│   └── InteractiveEditor/ # Material editor showcase
└── docs/                  # Documentation
```

## Design Principles

1. **Separation of Concerns**: Each system has a single, well-defined responsibility
2. **Loose Coupling**: Systems communicate through events and service interfaces
3. **Data-Driven Design**: Behavior controlled through configuration files
4. **Composition over Inheritance**: ECS pattern for game objects
5. **Developer Experience**: Comprehensive logging, debugging, and tooling

## Next Steps (Phase 2)

1. **Enhanced ECS**: System processors, more component types
2. **Advanced Rendering**: Pixel-perfect pipeline, lighting improvements
3. **Material System**: Data-driven material definitions
4. **AI Framework**: Behavior trees, pathfinding
5. **Asset Pipeline**: Hot-reloading, asset database

## Migration Guide

For existing code using the old Engine singleton pattern:

```cpp
// Before
auto renderer = Engine::Instance().GetRenderer();
auto world = Engine::Instance().GetWorld();

// After
auto renderer = Services::GetRenderer();
auto world = Services::GetWorld();
```

Event handling:
```cpp
// Before: Direct method calls between systems

// After: Event-based communication
EventBus::Instance().Subscribe<MaterialChangedEvent>([](const MaterialChangedEvent& event) {
    // React to material changes
});
```