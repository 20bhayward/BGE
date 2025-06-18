# BGE Entity Component System (ECS)

## Overview

The BGE ECS is a high-performance, archetype-based Entity Component System designed to handle 100,000+ entities efficiently. It provides a data-oriented architecture that maximizes cache efficiency and enables parallel processing.

## Key Features

- **Archetype-based Storage**: Entities with the same component combination are stored together for optimal cache performance
- **Generational Entity IDs**: Prevents use-after-free bugs with versioned entity identifiers
- **Structure-of-Arrays (SoA)**: Components are stored in contiguous arrays for efficient iteration
- **Zero-overhead Queries**: Fast entity filtering with compile-time optimizations
- **Type-safe API**: Strong typing prevents common errors
- **Extensible Component System**: Easy registration of custom components

## Quick Start

### Creating Entities

```cpp
auto& entityManager = EntityManager::Instance();

// Create an entity
EntityID player = entityManager.CreateEntity("Player");

// Add components
entityManager.AddComponent(player, TransformComponent{Vector3(0, 0, 0)});
entityManager.AddComponent(player, HealthComponent{100.0f});
entityManager.AddComponent(player, PhysicsComponent{});
```

### Querying Entities

```cpp
// Query all entities with Transform and Physics components
EntityQuery query(&entityManager);
query.With<TransformComponent>().With<PhysicsComponent>()
    .ForEach([](EntityID entity, TransformComponent& transform, PhysicsComponent& physics) {
        // Update physics based on transform
        physics.velocity = Vector3(1, 0, 0);
    });

// Query with filters
query.With<AIComponent>()
    .Where<AIComponent>([](const AIComponent& ai) {
        return ai.behavior == AIComponent::Behavior::Aggressive;
    })
    .ForEach([](EntityID entity, AIComponent& ai) {
        // Handle aggressive AI
    });
```

### Registering Custom Components

```cpp
struct CustomComponent {
    float value;
    std::string data;
};

// Register the component
ComponentRegistry::Instance().RegisterComponent<CustomComponent>("CustomComponent");
```

## Architecture

### Entity Storage

Entities are stored using an archetype-based approach:
- Entities with identical component sets share an archetype
- Components are stored in Structure-of-Arrays format within each archetype
- Entity movement between archetypes is optimized with edge caching

### Component Storage

Components are stored in contiguous arrays for each archetype:
- Direct memory access with zero indirection
- Cache-friendly iteration patterns
- Automatic memory management with pooling

### Query System

The query system provides efficient filtering:
- Archetype-level filtering eliminates unnecessary iteration
- Component existence checked via bitmasks
- Optional value-based filtering with predicates

## Performance Characteristics

- **Entity Creation**: < 100 nanoseconds
- **Component Access**: < 10 nanoseconds
- **Query Iteration**: > 1 million entities/second
- **Memory Overhead**: < 1KB per entity

## Best Practices

1. **Batch Operations**: Create entities and components in batches for better performance
2. **Query Caching**: Store query results when iterating multiple times
3. **Component Granularity**: Keep components small and focused
4. **Avoid Singleton Components**: Use global systems instead
5. **Profile Archetype Distribution**: Monitor archetype fragmentation

## Migration from Legacy System

The legacy Entity/Component system is still supported through compatibility wrappers:

```cpp
// Old API (still works)
Entity* entity = EntityManager::Instance().CreateEntity();
entity->AddComponent<TransformComponent>();

// New API (recommended)
EntityID entity = EntityManager::Instance().CreateEntity();
EntityManager::Instance().AddComponent(entity, TransformComponent{});
```

## Thread Safety

- Entity creation/destruction is thread-safe with internal synchronization
- Component access within systems should be synchronized by the job system
- Queries can be executed in parallel on different archetypes

## Future Improvements

- [ ] Spatial hashing for position-based queries
- [ ] Component change tracking for reactive systems
- [ ] Network serialization support
- [ ] GPU-compatible component layouts
- [ ] Dynamic component registration from scripts