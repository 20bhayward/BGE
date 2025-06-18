#include <iostream>
#include <chrono>
#include <random>
#include <thread>
#include "Core/ECS/EntityManager.h"
#include "Core/ECS/SystemManager.h"
#include "Core/ECS/Systems/MovementSystem.h"
#include "Core/Components.h"
#include "Core/Logger.h"

using namespace BGE;
using namespace std::chrono;

// Simple gravity system
class GravitySystem : public ComponentSystem<TransformComponent, VelocityComponent> {
public:
    GravitySystem() {
        SetName("GravitySystem");
        SetStage(SystemStage::Update);
        SetPriority(50); // Before movement
        DependsOn<MovementSystem>(); // Example dependency
    }
    
protected:
    void OnUpdateEntity(EntityID /*entity*/, TransformComponent& transform, VelocityComponent& velocity) override {
        // Apply gravity if not on ground
        if (transform.position.y > 0.0f) {
            velocity.acceleration.y = -9.81f; // Gravity
        } else {
            // On ground - stop falling
            transform.position.y = 0.0f;
            velocity.velocity.y = std::max(0.0f, velocity.velocity.y);
            velocity.acceleration.y = 0.0f;
        }
        
        // Update velocity from acceleration
        velocity.velocity += velocity.acceleration * m_deltaTime;
        
        // Apply damping
        velocity.velocity *= velocity.damping;
    }
    
    void OnUpdate(float deltaTime) override {
        m_deltaTime = deltaTime;
        ComponentSystem::OnUpdate(deltaTime);
    }
    
private:
    float m_deltaTime = 0.0f;
};

// Debug render system
class DebugRenderSystem : public ComponentSystem<TransformComponent> {
public:
    DebugRenderSystem() {
        SetName("DebugRenderSystem");
        SetStage(SystemStage::PreRender);
        SetPriority(1000);
    }
    
protected:
    void OnUpdateEntity(EntityID /*entity*/, TransformComponent& /*transform*/) override {
        // In real engine, would submit debug draw commands
        m_entityCount++;
    }
    
    void OnUpdate(float /*deltaTime*/) override {
        m_entityCount = 0;
        
        // Direct query to verify the system works
        GetQuery()->ForEach<TransformComponent>([this](EntityID entity, TransformComponent& transform) {
            OnUpdateEntity(entity, transform);
        });
        
        if (m_frameCounter % 60 == 0) { // Log every 60 frames
            std::cout << "Rendering " << m_entityCount << " entities\n";
        }
        m_frameCounter++;
    }
    
private:
    int m_entityCount = 0;
    int m_frameCounter = 0;
};

void CreateBouncingBalls(int count) {
    auto& entityManager = EntityManager::Instance();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> posX(-50.0f, 50.0f);
    std::uniform_real_distribution<float> posY(10.0f, 100.0f);
    std::uniform_real_distribution<float> velX(-10.0f, 10.0f);
    std::uniform_real_distribution<float> velY(-5.0f, 5.0f);
    
    for (int i = 0; i < count; ++i) {
        EntityID ball = entityManager.CreateEntity("Ball_" + std::to_string(i));
        
        // Add transform
        TransformComponent transform;
        transform.position = Vector3(posX(gen), posY(gen), 0.0f);
        auto transformResult = entityManager.AddComponent(ball, std::move(transform));
        if (!transformResult && i == 0) {
            std::cout << "Failed to add TransformComponent!\n";
        }
        
        // Add velocity
        VelocityComponent velocity;
        velocity.velocity = Vector3(velX(gen), velY(gen), 0.0f);
        velocity.damping = 0.99f;
        entityManager.AddComponent(ball, std::move(velocity));
    }
}

int main() {
    std::cout << "BGE Systems Framework Demo\n";
    std::cout << "==========================\n\n";
    
    try {
        // Register components first
        auto& registry = ComponentRegistry::Instance();
        auto transformId = registry.RegisterComponent<TransformComponent>("TransformComponent");
        auto velocityId = registry.RegisterComponent<VelocityComponent>("VelocityComponent");
        auto nameId = registry.RegisterComponent<NameComponent>("NameComponent");
        
        std::cout << "Registered components:\n";
        std::cout << "  TransformComponent ID: " << transformId << "\n";
        std::cout << "  VelocityComponent ID: " << velocityId << "\n";
        std::cout << "  NameComponent ID: " << nameId << "\n\n";
        
        auto& systemManager = SystemManager::Instance();
        
        // Register systems
        systemManager.RegisterSystem<GravitySystem>();
        systemManager.RegisterSystem<MovementSystem>();
        systemManager.RegisterSystem<DebugRenderSystem>();
        
        // Create entities
        std::cout << "Creating 1000 bouncing balls...\n";
        std::cout.flush();
        CreateBouncingBalls(1000);
        std::cout << "Entities created successfully.\n";
        
        // Debug: Check archetype state
        auto& entityManager = EntityManager::Instance();
        auto& archetypeManager = entityManager.GetArchetypeManager();
        std::cout << "\nArchetype state after entity creation:\n";
        for (size_t i = 0; i < archetypeManager.GetAllArchetypes().size(); ++i) {
            auto* archetype = archetypeManager.GetArchetype(static_cast<uint32_t>(i));
            if (archetype && archetype->GetEntityCount() > 0) {
                std::cout << "  Archetype " << i << ": " << archetype->GetEntityCount() << " entities\n";
            }
        }
        std::cout.flush();
        
        // Simulation loop
        std::cout << "\nRunning simulation for 5 seconds...\n";
        std::cout.flush();
        auto startTime = high_resolution_clock::now();
        auto lastTime = startTime;
        int frameCount = 0;
        
        while (true) {
            auto currentTime = high_resolution_clock::now();
            auto elapsed = duration_cast<duration<float>>(currentTime - startTime).count();
            auto deltaTime = duration_cast<duration<float>>(currentTime - lastTime).count();
            lastTime = currentTime;
            
            if (elapsed > 5.0f) break;
            
            // Update all systems
            systemManager.Update(deltaTime);
            
            frameCount++;
            if (frameCount % 30 == 0) {
                std::cout << "Frame " << frameCount << ", elapsed: " << elapsed << "s\n";
                std::cout.flush();
            }
            
            // Simulate frame rate
            std::this_thread::sleep_for(milliseconds(16)); // ~60 FPS
        }
        
        // Performance report
        std::cout << "\nSystem Performance Report:\n";
        std::cout << "-------------------------\n";
        
        for (const System* system : systemManager.GetAllSystems()) {
            std::cout << system->GetName() << " - Stage: " 
                     << static_cast<int>(system->GetStage()) 
                     << ", Priority: " << system->GetPriority() << "\n";
        }
        
        // Cleanup
        EntityManager::Instance().Clear();
        systemManager.Clear();
        
        std::cout << "\nSystems demo completed successfully!\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}