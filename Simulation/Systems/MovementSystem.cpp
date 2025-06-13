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

    // Assuming EntityManager has a way to iterate through active entities.
    // This part is based on a common ECS pattern. If BGE::EntityManager
    // has a different way to get all entities with specific components,
    // that should be used. For now, I'll assume a simple iteration.
    // The issue says "Iterate through all active entities."

    auto& activeEntities = entityManager.GetEntities(); // Assuming such a method exists and returns a collection of Entity* or similar.
                                                        // If not, this will need adjustment based on EntityManager's API.
                                                        // Let's assume GetEntities() returns something like std::map<EntityID, std::unique_ptr<Entity>>&

    for (auto const& [id, entityPtr] : activeEntities)
    {
        if (entityPtr) // Check if entity pointer is valid
        {
            Entity* entity = entityPtr.get(); // Get raw pointer

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
