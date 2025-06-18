#include "MovementSystem.h" // Local include
#include "../../Core/Entity.h" // For BGE::Entity, used when iterating
#include "../../Core/ECS/EntityManager.h" // For EntityManager
#include "../../Core/Components.h" // For component types

// Potentially needed for logging or assertions, if used.
// #include "Core/Logger.h"

namespace BGE
{

MovementSystem::MovementSystem()
{
    // Constructor
}

MovementSystem::~MovementSystem()
{
    // Destructor
}

void MovementSystem::Update(float deltaTime)
{
    // Use the legacy compatibility approach for now
    // In a full implementation, this would use the new query system
    auto& entityManager = EntityManager::Instance();
    
    // Get all legacy entities and check for components
    auto allEntities = entityManager.GetAllEntities();
    
    for (Entity* entity : allEntities)
    {
        if (entity && entity->IsActive()) // Check if entity pointer is valid and active
        {
            // Check if the entity possesses both a TransformComponent AND a VelocityComponent.
            TransformComponent* transform = entity->GetComponent<TransformComponent>();
            VelocityComponent* velocity = entity->GetComponent<VelocityComponent>();

            if (transform && velocity)
            {
                // Update the transform->position by adding the velocity->velocity * deltaTime.
                transform->position.x += velocity->velocity.x * deltaTime;
                transform->position.y += velocity->velocity.y * deltaTime;
                transform->position.z += velocity->velocity.z * deltaTime;
            }
        }
    }
}

const char* MovementSystem::GetName() const
{
    return "MovementSystem";
}

} // namespace BGE
