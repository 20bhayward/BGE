#include "SystemManager.h"
#include "ISystem.h" // Technically included via SystemManager.h, but good for clarity.

namespace BGE
{

SystemManager::SystemManager()
{
    // Constructor, if any specific initialization is needed.
}

SystemManager::~SystemManager()
{
    // If using raw pointers (ISystem*), and SystemManager is responsible for deleting them:
    // for (ISystem* system : m_systems) {
    //     delete system;
    // }
    // m_systems.clear();
    // However, if systems are owned elsewhere (e.g., by Engine using unique_ptr for each system,
    // and then registered here as raw pointers), then SystemManager should not delete them.
    // The issue description implies Engine will create MovementSystem and register it,
    // suggesting Engine might own it. For now, leave destructor empty and clarify ownership if issues arise.
    // Based on "std::unique_ptr<SystemManager> as a private member of the Engine class" and
    // "create an instance of your new MovementSystem and register it", it seems Engine will own MovementSystem.
}

void SystemManager::RegisterSystem(ISystem* system)
{
    if (system)
    {
        m_systems.push_back(system);
    }
}

void SystemManager::UpdateAll(float deltaTime)
{
    for (ISystem* system : m_systems)
    {
        if (system) // Good practice to check pointer before dereferencing
        {
            system->Update(deltaTime);
        }
    }
}

} // namespace BGE
