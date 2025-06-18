#include "MovementSystem.h"
#include "../../Core/ECS/EntityManager.h"
#include "../../Core/ECS/EntityQuery.h"
#include "../../Core/Components.h"

namespace BGE {

// Alternative implementation using the new ECS query system
class MovementSystemECS : public MovementSystem {
public:
    void Update(float deltaTime) override {
        auto& entityManager = EntityManager::Instance();
        
        // Create a query for entities with both Transform and Velocity components
        EntityQuery query(&entityManager);
        query.With<TransformComponent>()
             .With<VelocityComponent>()
             .ForEach([deltaTime](EntityID entity, TransformComponent& transform, VelocityComponent& velocity) {
                 // Update position based on velocity
                 transform.position.x += velocity.velocity.x * deltaTime;
                 transform.position.y += velocity.velocity.y * deltaTime;
                 transform.position.z += velocity.velocity.z * deltaTime;
             });
    }
    
    const char* GetName() const override {
        return "MovementSystemECS";
    }
};

} // namespace BGE