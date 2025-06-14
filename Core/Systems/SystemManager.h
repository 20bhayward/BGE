#pragma once

#include "ISystem.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <algorithm>

namespace BGE {

class SystemManager {
public:
    static SystemManager& Instance();
    
    // System registration
    template<typename T>
    void RegisterSystem(std::shared_ptr<T> system) {
        static_assert(std::is_base_of_v<ISystem, T>, "T must inherit from ISystem");
        
        auto typeId = std::type_index(typeid(T));
        m_systems[typeId] = system;
        m_orderedSystems.push_back(system);
        
        // Sort systems by priority
        SortSystemsByPriority();
        
        // Initialize the system
        system->Initialize();
    }
    
    // System retrieval
    template<typename T>
    std::shared_ptr<T> GetSystem() {
        auto typeId = std::type_index(typeid(T));
        auto it = m_systems.find(typeId);
        if (it != m_systems.end()) {
            return std::static_pointer_cast<T>(it->second);
        }
        return nullptr;
    }
    
    // System removal
    template<typename T>
    void UnregisterSystem() {
        auto typeId = std::type_index(typeid(T));
        auto it = m_systems.find(typeId);
        if (it != m_systems.end()) {
            it->second->Shutdown();
            m_systems.erase(it);
            
            // Remove from ordered list
            m_orderedSystems.erase(
                std::remove_if(m_orderedSystems.begin(), m_orderedSystems.end(),
                    [typeId](const std::shared_ptr<ISystem>& sys) {
                        return std::type_index(typeid(*sys)) == typeId;
                    }
                ),
                m_orderedSystems.end()
            );
        }
    }
    
    // Update all systems
    void UpdateSystems(float deltaTime);
    
    // System dependency management
    void HandleSystemDependencies();
    
    // Clear all systems
    void Clear();
    
    // Get all systems (for debugging/tools)
    const std::vector<std::shared_ptr<ISystem>>& GetAllSystems() const { 
        return m_orderedSystems; 
    }
    
private:
    SystemManager() = default;
    ~SystemManager();
    
    void SortSystemsByPriority();
    
    std::unordered_map<std::type_index, std::shared_ptr<ISystem>> m_systems;
    std::vector<std::shared_ptr<ISystem>> m_orderedSystems;
};

} // namespace BGE