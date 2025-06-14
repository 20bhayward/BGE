#pragma once

#include <vector>
#include <memory> // For std::unique_ptr if chosen, or for general good practice.
#include "ISystem.h" // Needs to know the ISystem interface.

namespace BGE
{

class SystemManager
{
public:
    SystemManager();
    ~SystemManager();

    void RegisterSystem(ISystem* system); // Using raw pointer as per plan, consider ownership.
                                         // If SystemManager owns the systems, std::vector<std::unique_ptr<ISystem>> would be better.
                                         // For now, sticking to ISystem* as specified.

    void UpdateAll(float deltaTime);

private:
    std::vector<ISystem*> m_systems;
};

} // namespace BGE
