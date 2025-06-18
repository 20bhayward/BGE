#pragma once

#include "System.h"
#include "../Logger.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <typeindex>

namespace BGE {

class SystemManager {
public:
    static SystemManager& Instance() {
        static SystemManager instance;
        return instance;
    }
    
    // Register a system
    template<typename T, typename... Args>
    T* RegisterSystem(Args&&... args) {
        static_assert(std::is_base_of_v<System, T>, "T must derive from System");
        
        auto typeIndex = std::type_index(typeid(T));
        
        // Check if already registered
        auto it = m_systems.find(typeIndex);
        if (it != m_systems.end()) {
            BGE_LOG_WARNING("SystemManager", "System already registered: " + std::string(typeid(T).name()));
            return static_cast<T*>(it->second.get());
        }
        
        // Create system
        auto system = std::make_unique<T>(std::forward<Args>(args)...);
        T* systemPtr = system.get();
        
        // Store system
        m_systems[typeIndex] = std::move(system);
        m_orderedSystems.push_back(systemPtr);
        m_needsSort = true;
        
        // Initialize
        systemPtr->OnCreate();
        if (systemPtr->IsEnabled()) {
            systemPtr->OnStart();
        }
        
        BGE_LOG_INFO("SystemManager", "Registered system: " + std::string(systemPtr->GetName()));
        
        return systemPtr;
    }
    
    // Unregister a system
    template<typename T>
    void UnregisterSystem() {
        auto typeIndex = std::type_index(typeid(T));
        auto it = m_systems.find(typeIndex);
        
        if (it != m_systems.end()) {
            System* system = it->second.get();
            
            // Remove from ordered list
            m_orderedSystems.erase(
                std::remove(m_orderedSystems.begin(), m_orderedSystems.end(), system),
                m_orderedSystems.end()
            );
            
            // Cleanup
            if (system->IsEnabled()) {
                system->OnStop();
            }
            system->OnDestroy();
            
            BGE_LOG_INFO("SystemManager", "Unregistered system: " + std::string(system->GetName()));
            
            // Remove from map
            m_systems.erase(it);
        }
    }
    
    // Get a system
    template<typename T>
    T* GetSystem() {
        auto typeIndex = std::type_index(typeid(T));
        auto it = m_systems.find(typeIndex);
        return it != m_systems.end() ? static_cast<T*>(it->second.get()) : nullptr;
    }
    
    // Update all systems
    void Update(float deltaTime) {
        if (m_needsSort) {
            SortSystems();
        }
        
        // Update systems by stage
        for (uint32_t stage = 0; stage < static_cast<uint32_t>(SystemStage::Count); ++stage) {
            UpdateStage(static_cast<SystemStage>(stage), deltaTime);
        }
    }
    
    // Update specific stage
    void UpdateStage(SystemStage stage, float deltaTime) {
        // TODO: Add parallel execution support for SystemMode::Parallel
        
        for (System* system : m_orderedSystems) {
            if (system->IsEnabled() && system->GetStage() == stage) {
                system->Update(deltaTime);
            }
        }
    }
    
    // Enable/disable all systems
    void SetAllSystemsEnabled(bool enabled) {
        for (auto& [type, system] : m_systems) {
            system->SetEnabled(enabled);
        }
    }
    
    // Get all systems
    const std::vector<System*>& GetAllSystems() const {
        return m_orderedSystems;
    }
    
    // Clear all systems
    void Clear() {
        // Disable and destroy in reverse order
        for (auto it = m_orderedSystems.rbegin(); it != m_orderedSystems.rend(); ++it) {
            System* system = *it;
            if (system->IsEnabled()) {
                system->OnStop();
            }
            system->OnDestroy();
        }
        
        m_orderedSystems.clear();
        m_systems.clear();
        m_needsSort = false;
    }
    
private:
    SystemManager() = default;
    ~SystemManager() {
        Clear();
    }
    
    void SortSystems() {
        // Sort by stage, then priority, then handle dependencies
        std::sort(m_orderedSystems.begin(), m_orderedSystems.end(),
            [](const System* a, const System* b) {
                if (a->GetStage() != b->GetStage()) {
                    return static_cast<uint32_t>(a->GetStage()) < static_cast<uint32_t>(b->GetStage());
                }
                return a->GetPriority() < b->GetPriority();
            });
        
        // TODO: Topological sort for dependencies within same stage
        
        m_needsSort = false;
    }
    
    std::unordered_map<std::type_index, std::unique_ptr<System>> m_systems;
    std::vector<System*> m_orderedSystems;
    bool m_needsSort = false;
};

// Helper macro for system registration
#define REGISTER_SYSTEM(Type, ...) \
    BGE::SystemManager::Instance().RegisterSystem<Type>(__VA_ARGS__)

} // namespace BGE