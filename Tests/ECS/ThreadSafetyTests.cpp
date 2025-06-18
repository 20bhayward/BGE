#include <gtest/gtest.h>
#include "../../Core/ECS/EntityManager.h"
#include "../../Core/ECS/Components/CoreComponents.h"
#include <thread>
#include <vector>
#include <atomic>
#include <random>
#include <chrono>

using namespace BGE;

class ECSThreadSafetyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Register core components
        RegisterCoreComponents();
        
        // Clear any existing entities
        EntityManager::Instance().Clear();
    }
    
    void TearDown() override {
        EntityManager::Instance().Clear();
    }
};

// Test concurrent entity creation
TEST_F(ECSThreadSafetyTest, ConcurrentEntityCreation) {
    const int NUM_THREADS = 8;
    const int ENTITIES_PER_THREAD = 1000;
    
    std::vector<std::thread> threads;
    std::vector<std::vector<EntityID>> threadEntities(NUM_THREADS);
    
    // Create entities from multiple threads
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([t, &threadEntities]() {
            auto& manager = EntityManager::Instance();
            for (int i = 0; i < ENTITIES_PER_THREAD; ++i) {
                EntityID entity = manager.CreateEntity("Thread" + std::to_string(t) + "_Entity" + std::to_string(i));
                threadEntities[t].push_back(entity);
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify all entities are valid
    auto& manager = EntityManager::Instance();
    EXPECT_EQ(manager.GetEntityCount(), NUM_THREADS * ENTITIES_PER_THREAD);
    
    for (const auto& entities : threadEntities) {
        for (EntityID entity : entities) {
            EXPECT_TRUE(manager.IsEntityValid(entity));
        }
    }
}

// Test concurrent component operations
TEST_F(ECSThreadSafetyTest, ConcurrentComponentOperations) {
    const int NUM_ENTITIES = 100;
    const int NUM_THREADS = 4;
    
    // Create entities
    std::vector<EntityID> entities;
    auto& manager = EntityManager::Instance();
    for (int i = 0; i < NUM_ENTITIES; ++i) {
        entities.push_back(manager.CreateEntity("Entity" + std::to_string(i)));
    }
    
    std::atomic<int> successCount{0};
    std::vector<std::thread> threads;
    
    // Add components from multiple threads
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([&entities, &manager, &successCount, t]() {
            std::mt19937 rng(t);
            std::uniform_int_distribution<int> dist(0, NUM_ENTITIES - 1);
            
            for (int i = 0; i < 250; ++i) {
                int idx = dist(rng);
                EntityID entity = entities[idx];
                
                // Randomly add different component types
                switch (i % 3) {
                    case 0:
                        if (manager.AddComponent(entity, TransformComponent{Vector3(i, i, i)})) {
                            successCount++;
                        }
                        break;
                    case 1:
                        if (manager.AddComponent(entity, VelocityComponent{Vector3(i, 0, 0)})) {
                            successCount++;
                        }
                        break;
                    case 2:
                        if (manager.AddComponent(entity, HealthComponent{static_cast<float>(i)})) {
                            successCount++;
                        }
                        break;
                }
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify components were added
    EXPECT_GT(successCount.load(), 0);
    
    // Verify data integrity
    for (EntityID entity : entities) {
        EXPECT_TRUE(manager.IsEntityValid(entity));
    }
}

// Test concurrent entity destruction
TEST_F(ECSThreadSafetyTest, ConcurrentEntityDestruction) {
    const int NUM_ENTITIES = 1000;
    
    // Create entities
    std::vector<EntityID> entities;
    auto& manager = EntityManager::Instance();
    for (int i = 0; i < NUM_ENTITIES; ++i) {
        EntityID entity = manager.CreateEntity();
        manager.AddComponent(entity, TransformComponent{});
        entities.push_back(entity);
    }
    
    std::atomic<int> destroyedCount{0};
    std::vector<std::thread> threads;
    
    // Destroy entities from multiple threads
    const int THREADS = 4;
    const int ENTITIES_PER_THREAD = NUM_ENTITIES / THREADS;
    
    for (int t = 0; t < THREADS; ++t) {
        threads.emplace_back([&manager, &entities, &destroyedCount, t, ENTITIES_PER_THREAD]() {
            int start = t * ENTITIES_PER_THREAD;
            int end = start + ENTITIES_PER_THREAD;
            
            for (int i = start; i < end; ++i) {
                manager.DestroyEntity(entities[i]);
                destroyedCount++;
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify all entities were destroyed
    EXPECT_EQ(destroyedCount.load(), NUM_ENTITIES);
    EXPECT_EQ(manager.GetEntityCount(), 0);
    
    // Verify entities are invalid
    for (EntityID entity : entities) {
        EXPECT_FALSE(manager.IsEntityValid(entity));
    }
}

// Test mixed operations under contention
TEST_F(ECSThreadSafetyTest, MixedOperationsUnderContention) {
    const int DURATION_MS = 1000; // Run for 1 second
    std::atomic<bool> shouldStop{false};
    std::atomic<int> operations{0};
    
    auto& manager = EntityManager::Instance();
    
    // Thread pool for various operations
    std::vector<std::thread> threads;
    
    // Creator thread
    threads.emplace_back([&manager, &shouldStop, &operations]() {
        while (!shouldStop) {
            EntityID entity = manager.CreateEntity();
            manager.AddComponent(entity, TransformComponent{});
            operations++;
        }
    });
    
    // Destroyer thread
    threads.emplace_back([&manager, &shouldStop, &operations]() {
        while (!shouldStop) {
            EntityQuery query(&manager);
            auto result = query.With<TransformComponent>().Execute();
            
            for (auto data : result) {
                if (shouldStop) break;
                manager.DestroyEntity(data.entity);
                operations++;
            }
        }
    });
    
    // Component modifier thread
    threads.emplace_back([&manager, &shouldStop, &operations]() {
        while (!shouldStop) {
            EntityQuery query(&manager);
            auto result = query.With<TransformComponent>().Execute();
            
            for (auto data : result) {
                if (shouldStop) break;
                manager.AddComponent(data.entity, VelocityComponent{});
                operations++;
            }
        }
    });
    
    // Query thread
    threads.emplace_back([&manager, &shouldStop, &operations]() {
        while (!shouldStop) {
            EntityQuery query(&manager);
            auto result = query.With<TransformComponent>().Execute();
            
            size_t count = 0;
            for (auto data : result) {
                count++;
            }
            operations++;
        }
    });
    
    // Run for specified duration
    std::this_thread::sleep_for(std::chrono::milliseconds(DURATION_MS));
    shouldStop = true;
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify system is still in valid state
    EXPECT_GE(operations.load(), 0);
    
    // Final validation
    EntityQuery finalQuery(&manager);
    auto finalResult = finalQuery.Execute();
    size_t finalCount = 0;
    for (auto data : finalResult) {
        EXPECT_TRUE(manager.IsEntityValid(data.entity));
        finalCount++;
    }
    
    std::cout << "Performed " << operations.load() << " operations" << std::endl;
    std::cout << "Final entity count: " << manager.GetEntityCount() << std::endl;
}

// Test query consistency during modifications
TEST_F(ECSThreadSafetyTest, QueryConsistencyDuringModifications) {
    const int NUM_ENTITIES = 500;
    
    auto& manager = EntityManager::Instance();
    
    // Create initial entities
    for (int i = 0; i < NUM_ENTITIES; ++i) {
        EntityID entity = manager.CreateEntity();
        manager.AddComponent(entity, TransformComponent{Vector3(i, i, i)});
    }
    
    std::atomic<bool> shouldStop{false};
    std::atomic<int> inconsistencies{0};
    
    // Query thread - continuously queries and validates
    std::thread queryThread([&manager, &shouldStop, &inconsistencies]() {
        while (!shouldStop) {
            EntityQuery query(&manager);
            auto result = query.With<TransformComponent>().Execute();
            
            for (auto data : result) {
                auto* transform = manager.GetComponent<TransformComponent>(data.entity);
                if (!transform) {
                    inconsistencies++;
                }
            }
        }
    });
    
    // Modification thread - adds/removes components
    std::thread modThread([&manager, &shouldStop]() {
        EntityQuery query(&manager);
        
        while (!shouldStop) {
            auto result = query.With<TransformComponent>().Execute();
            
            int count = 0;
            for (auto data : result) {
                if (count++ % 2 == 0) {
                    manager.AddComponent(data.entity, VelocityComponent{});
                } else {
                    manager.RemoveComponent<VelocityComponent>(data.entity);
                }
                
                if (shouldStop) break;
            }
        }
    });
    
    // Run for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    shouldStop = true;
    
    queryThread.join();
    modThread.join();
    
    // Should have no inconsistencies
    EXPECT_EQ(inconsistencies.load(), 0);
}

// Performance test - measure operation throughput
TEST_F(ECSThreadSafetyTest, PerformanceUnderContention) {
    const int NUM_THREADS = 8;
    const int OPERATIONS_PER_THREAD = 10000;
    
    auto& manager = EntityManager::Instance();
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<std::thread> threads;
    
    // Create threads performing mixed operations
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([&manager, t]() {
            for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
                EntityID entity = manager.CreateEntity();
                manager.AddComponent(entity, TransformComponent{});
                
                if (i % 10 == 0) {
                    manager.DestroyEntity(entity);
                }
            }
        });
    }
    
    // Wait for completion
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    int totalOperations = NUM_THREADS * OPERATIONS_PER_THREAD * 2; // Create + AddComponent
    double opsPerSecond = (totalOperations * 1000.0) / duration.count();
    
    std::cout << "Thread safety performance test:" << std::endl;
    std::cout << "  Total operations: " << totalOperations << std::endl;
    std::cout << "  Duration: " << duration.count() << "ms" << std::endl;
    std::cout << "  Operations/second: " << opsPerSecond << std::endl;
    
    // Verify reasonable performance (should handle at least 100k ops/sec even with locking)
    EXPECT_GT(opsPerSecond, 100000);
}