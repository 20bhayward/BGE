#pragma once

#include "../../Core/ISystem.h" // Relative path to ISystem interface
#include "../../Core/Entity.h" // For BGE::EntityManager
#include "../../Core/Components.h"  // For BGE::TransformComponent and BGE::VelocityComponent
                              // Also includes BGE::Vector3 via Components.h typically

namespace BGE
{

class MovementSystem : public ISystem
{
public:
    MovementSystem();
    ~MovementSystem() override;

    void Update(float deltaTime) override;
    const char* GetName() const override;
};

} // namespace BGE
