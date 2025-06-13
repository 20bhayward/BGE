#include "MovementSystem.h" // Local include
#include "../../Core/Entity.h" // For BGE::Entity, used when iterating

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
    EntityManager& entityManager = EntityManager::Instance();

    // Get all entities that have TransformComponent
    // We'll check for VelocityComponent inside the loop
    std::vector<Entity*> entities = entityManager.GetEntitiesWithComponent<TransformComponent>();

    for (Entity* entity : entities)
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
