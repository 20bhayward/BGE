#pragma once

#include "EntityManager.h"
#include "EntityQuery.h"
#include "../ISystem.h"
#include <vector>
#include <unordered_set>
#include <typeindex>
#include <memory>

namespace BGE {

// System update stages for execution ordering
enum class SystemStage : uint32_t {
    PreUpdate = 0,      // Input handling, event processing
    Update = 1,         // Core game logic
    LateUpdate = 2,     // Camera, UI updates
    PreRender = 3,      // Prepare rendering data
    PostRender = 4,     // Cleanup, stats
    
    Count
};

// System execution mode
enum class SystemMode {
    SingleThreaded,     // Run on main thread
    Parallel,          // Can run in parallel with other parallel systems
    Exclusive          // Must run alone (modifies singleton resources)
};

// Base class for all ECS systems
class System : public ISystem {
public:
    System() = default;
    virtual ~System() = default;
    
    // ISystem interface
    virtual void Update(float deltaTime) override { OnUpdate(deltaTime); }
    virtual const char* GetName() const override { return m_name.c_str(); }
    
    // ECS System interface
    virtual void OnCreate() {}                    // Called when system is added
    virtual void OnDestroy() {}                   // Called when system is removed
    virtual void OnStart() {}                     // Called when system is enabled
    virtual void OnStop() {}                      // Called when system is disabled
    virtual void OnUpdate(float deltaTime) = 0;   // Main update function
    
    // System properties
    SystemStage GetStage() const { return m_stage; }
    SystemMode GetMode() const { return m_mode; }
    bool IsEnabled() const { return m_enabled; }
    uint32_t GetPriority() const { return m_priority; }
    
    void SetEnabled(bool enabled) { 
        if (m_enabled != enabled) {
            m_enabled = enabled;
            if (enabled) OnStart();
            else OnStop();
        }
    }
    
    // Dependencies
    void DependsOn(std::type_index systemType) {
        m_dependencies.insert(systemType);
    }
    
    template<typename T>
    void DependsOn() {
        DependsOn(std::type_index(typeid(T)));
    }
    
    const std::unordered_set<std::type_index>& GetDependencies() const {
        return m_dependencies;
    }
    
protected:
    // Configuration (set in derived constructor)
    void SetName(const std::string& name) { m_name = name; }
    void SetStage(SystemStage stage) { m_stage = stage; }
    void SetMode(SystemMode mode) { m_mode = mode; }
    void SetPriority(uint32_t priority) { m_priority = priority; }
    
    // Helper to get entity manager
    EntityManager& GetEntityManager() { return EntityManager::Instance(); }
    
private:
    std::string m_name = "UnnamedSystem";
    SystemStage m_stage = SystemStage::Update;
    SystemMode m_mode = SystemMode::SingleThreaded;
    uint32_t m_priority = 1000; // Lower = earlier execution
    bool m_enabled = true;
    std::unordered_set<std::type_index> m_dependencies;
};

// System that automatically queries entities with specific components
template<typename... Components>
class ComponentSystem : public System {
public:
    ComponentSystem() {
        // Build query mask for required components
        m_query = std::make_unique<EntityQuery>(&EntityManager::Instance());
        (m_query->With<Components>(), ...);
    }
    
protected:
    // Override this to process entities
    virtual void OnUpdateEntity(EntityID entity, Components&... components) = 0;
    
    // Default implementation queries and processes all matching entities
    virtual void OnUpdate(float deltaTime) override {
        m_query->ForEach<Components...>([this, deltaTime](EntityID entity, Components&... components) {
            OnUpdateEntity(entity, components...);
        });
    }
    
    // Get the query for custom processing
    EntityQuery* GetQuery() { return m_query.get(); }
    
private:
    std::unique_ptr<EntityQuery> m_query;
};

} // namespace BGE