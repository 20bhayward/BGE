#include <iostream>
#include <chrono>
#include "Core/ECS/EntityManager.h"
#include "Core/ECS/EntityQuery.h"
#include "Core/ECS/ComponentRegistry.h"

using namespace BGE;
using namespace std::chrono;

// Simple test component
struct TestComponent {
    float value = 0.0f;
    int counter = 0;
};

void BenchmarkECS() {
    std::cout << "BGE ECS Performance Benchmark\n";
    std::cout << "=============================\n\n";
    
    // Component already registered in main()
    auto& registry = ComponentRegistry::Instance();
    std::cout << "Registered TestComponent with TypeID: " << registry.GetComponentTypeID<TestComponent>() << "\n";
    
    auto& entityManager = EntityManager::Instance();
    
    // Benchmark entity creation
    const int ENTITY_COUNT = 100000;
    std::cout << "Creating " << ENTITY_COUNT << " entities...\n";
    
    auto start = high_resolution_clock::now();
    
    std::vector<EntityID> entities;
    entities.reserve(ENTITY_COUNT);
    
    for (int i = 0; i < ENTITY_COUNT; ++i) {
        entities.push_back(entityManager.CreateEntity("Entity_" + std::to_string(i)));
    }
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    
    std::cout << "Entity creation time: " << duration.count() << "ms\n";
    std::cout << "Average per entity: " << (duration.count() * 1000.0 / ENTITY_COUNT) << "μs\n\n";
    
    // Benchmark component addition
    std::cout << "Adding components to entities...\n";
    start = high_resolution_clock::now();
    
    // Add test components
    for (int i = 0; i < ENTITY_COUNT; ++i) {
        EntityID entity = entities[i];
        
        // Add test component to all entities
        TestComponent testComp;
        testComp.value = static_cast<float>(i);
        testComp.counter = i;
        auto addedResult = entityManager.AddComponent(entity, std::move(testComp));
        auto* added = addedResult ? addedResult.GetValue() : nullptr;
        if (i == 0) {
            std::cout << "First component add result: " << (added ? "success" : "failed") << "\n";
            std::cout << "Entity ID: " << entity.id << " (index: " << entity.GetIndex() << ", gen: " << entity.GetGeneration() << ")\n";
            if (!added) {
                std::cout << "Failed to add component to first entity!\n";
                break;
            }
        }
    }
    
    end = high_resolution_clock::now();
    duration = duration_cast<milliseconds>(end - start);
    
    std::cout << "Component addition time: " << duration.count() << "ms\n\n";
    
    // Benchmark queries
    std::cout << "Running queries...\n";
    
    // Query 1: All entities with TestComponent
    start = high_resolution_clock::now();
    size_t count = 0;
    
    EntityQuery query1(&entityManager);
    query1.With<TestComponent>().ForEach([&count](EntityID) {
        count++;
    });
    
    end = high_resolution_clock::now();
    auto queryDuration = duration_cast<microseconds>(end - start);
    
    std::cout << "Query 1 (TestComponent): " << count << " entities in " << queryDuration.count() << "μs\n";
    std::cout << "  Average per entity: " << (queryDuration.count() / (double)count) << "μs\n\n";
    
    // Test archetype statistics
    auto& archetypeManager = entityManager.GetArchetypeManager();
    std::cout << "Archetype Statistics:\n";
    std::cout << "  Total archetypes: " << archetypeManager.GetAllArchetypes().size() << "\n";
    
    for (size_t i = 0; i < archetypeManager.GetAllArchetypes().size(); ++i) {
        auto* archetype = archetypeManager.GetArchetype(static_cast<uint32_t>(i));
        if (archetype && archetype->GetEntityCount() > 0) {
            std::cout << "  Archetype " << i << ": " << archetype->GetEntityCount() << " entities";
            std::cout << " (Components:";
            for (auto compType : archetype->GetComponentTypes()) {
                auto* info = registry.GetComponentInfo(compType);
                if (info) {
                    std::cout << " " << info->name;
                }
            }
            std::cout << ")\n";
        }
    }
    
    std::cout << "\nMemory usage estimate:\n";
    std::cout << "  Entity records: " << (ENTITY_COUNT * sizeof(EntityRecord)) / 1024 << " KB\n";
    std::cout << "  Components (approx): " << (ENTITY_COUNT * 64) / 1024 << " KB\n";
    
    // Cleanup
    std::cout << "\nCleaning up...\n";
    start = high_resolution_clock::now();
    
    entityManager.Clear();
    
    end = high_resolution_clock::now();
    duration = duration_cast<milliseconds>(end - start);
    
    std::cout << "Cleanup time: " << duration.count() << "ms\n";
}

void DemoBasicUsage() {
    std::cout << "\n\nBasic ECS Usage Demo\n";
    std::cout << "===================\n\n";
    
    auto& entityManager = EntityManager::Instance();
    auto& registry = ComponentRegistry::Instance();
    
    // Check archetype state after Clear()
    auto& archetypeManager = entityManager.GetArchetypeManager();
    std::cout << "Archetypes after Clear(): " << archetypeManager.GetAllArchetypes().size() << "\n";
    
    // Check if TestComponent is registered
    auto typeId = registry.GetComponentTypeID<TestComponent>();
    std::cout << "TestComponent TypeID: " << typeId << "\n";
    
    // Create a test entity
    EntityID testEntity = entityManager.CreateEntity("TestEntity");
    std::cout << "Created entity with ID: " << testEntity.id << "\n";
    
    // Add test component
    TestComponent testComp;
    testComp.value = 42.0f;
    testComp.counter = 1337;
    auto addResult = entityManager.AddComponent(testEntity, std::move(testComp));
    auto* result = addResult ? addResult.GetValue() : nullptr;
    std::cout << "AddComponent result: " << (result ? "success" : "failed") << "\n";
    
    auto* comp = entityManager.GetComponent<TestComponent>(testEntity);
    if (comp) {
        std::cout << "Test entity has TestComponent with value: " << comp->value << " and counter: " << comp->counter << "\n";
    } else {
        std::cout << "Warning: Could not retrieve TestComponent!\n";
        
        // Debug archetype state
        std::cout << "Debug: Current archetypes after component add:\n";
        for (size_t i = 0; i < archetypeManager.GetAllArchetypes().size(); ++i) {
            auto* archetype = archetypeManager.GetArchetype(static_cast<uint32_t>(i));
            if (archetype) {
                std::cout << "  Archetype " << i << ": " << archetype->GetEntityCount() << " entities, component mask: ";
                for (size_t j = 0; j < 10; ++j) {
                    if (archetype->GetMask().test(j)) {
                        std::cout << j << " ";
                    }
                }
                std::cout << "\n";
            }
        }
    }
    
    // Query to verify
    std::cout << "Verifying with query...\n";
    EntityQuery verifyQuery(&entityManager);
    size_t verifyCount = 0;
    verifyQuery.With<TestComponent>().ForEach<TestComponent>([&verifyCount](EntityID, TestComponent& test) {
        verifyCount++;
        if (verifyCount == 1) {
            std::cout << "Found TestComponent via query with value: " << test.value << "\n";
        }
    });
    std::cout << "Total entities with TestComponent found: " << verifyCount << "\n";
    
    std::cout << "Basic usage demo completed!\n";
}

int main() {
    try {
        // Register component once at the start
        auto& registry = ComponentRegistry::Instance();
        auto typeId = registry.RegisterComponent<TestComponent>("TestComponent");
        std::cout << "Initial TestComponent registration: TypeID = " << typeId << "\n\n";
        
        BenchmarkECS();
        DemoBasicUsage();
        
        std::cout << "\nECS Demo completed successfully!\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}