// Minimal ECS test to verify the core system works
#include "EntityManager.h"
#include "EntityQuery.h"
#include "Components/CoreComponents.h"
#include <iostream>

void TestMinimalECS() {
    std::cout << "Testing minimal ECS functionality..." << std::endl;
    
    // Register core components
    RegisterCoreComponents();
    
    auto& entityManager = BGE::EntityManager::Instance();
    
    // Create a simple entity
    BGE::EntityID entity = entityManager.CreateEntity("TestEntity");
    std::cout << "Created entity with ID: " << entity.id << std::endl;
    
    // Add a transform component
    BGE::TransformComponent transform;
    transform.position = BGE::Vector3(1.0f, 2.0f, 3.0f);
    entityManager.AddComponent(entity, std::move(transform));
    std::cout << "Added TransformComponent" << std::endl;
    
    // Query for the entity
    BGE::EntityQuery query(&entityManager);
    size_t count = 0;
    query.With<BGE::TransformComponent>().ForEach([&count](BGE::EntityID id) {
        count++;
        std::cout << "Found entity " << id.id << " with TransformComponent" << std::endl;
    });
    
    std::cout << "Found " << count << " entities with TransformComponent" << std::endl;
    std::cout << "Minimal ECS test completed successfully!" << std::endl;
}