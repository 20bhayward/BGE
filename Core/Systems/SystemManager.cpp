#include "SystemManager.h"
#include "../Logger.h"

namespace BGE {

SystemManager& SystemManager::Instance() {
    static SystemManager instance;
    return instance;
}

SystemManager::~SystemManager() {
    Clear();
}

void SystemManager::UpdateSystems(float deltaTime) {
    for (auto& system : m_orderedSystems) {
        if (system->IsEnabled()) {
            system->Update(deltaTime);
        }
    }
}

void SystemManager::HandleSystemDependencies() {
    // TODO: Implement topological sort based on component dependencies
    // For now, we rely on priority-based ordering
    SortSystemsByPriority();
}

void SystemManager::Clear() {
    // Shutdown systems in reverse order
    for (auto it = m_orderedSystems.rbegin(); it != m_orderedSystems.rend(); ++it) {
        (*it)->Shutdown();
    }
    
    m_systems.clear();
    m_orderedSystems.clear();
}

void SystemManager::SortSystemsByPriority() {
    std::sort(m_orderedSystems.begin(), m_orderedSystems.end(),
        [](const std::shared_ptr<ISystem>& a, const std::shared_ptr<ISystem>& b) {
            return static_cast<int>(a->GetPriority()) < static_cast<int>(b->GetPriority());
        }
    );
    
    BGE_LOG_INFO("SystemManager", "Sorted " + std::to_string(m_orderedSystems.size()) + " systems by priority");
}

} // namespace BGE