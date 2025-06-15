#pragma once

#include <vector>
#include <memory> // For std::unique_ptr if ParticleSystem holds complex objects later
#include "../Core/Math/Vector2.h"
#include "../Core/Math/Vector3.h" // For color

// Forward declare Renderer if needed for drawing, or pass drawing context
// class Renderer;

namespace BGE {

// Structure to define properties for emitting a new particle
struct ParticleProperties {
    Vector2 position = {0.0f, 0.0f};
    Vector2 velocity = {0.0f, 0.0f};
    Vector3 color = {1.0f, 1.0f, 1.0f}; // Default to white
    float lifetime = 1.0f;             // Default to 1 second
    // Future: float size = 1.0f;
    // Future: float rotation = 0.0f;
    // Future: TextureID texture = 0;
};

// Represents a single particle in the system
struct Particle {
    Vector2 position;
    Vector2 velocity;
    Vector3 color;
    float lifetime;
    // float size;       // Future extension
    // float rotation;   // Future extension
    // TextureID texture; // Future extension
    bool isActive = false;
};

class ParticleSystem {
public:
    ParticleSystem();
    ~ParticleSystem();

    bool Initialize(size_t poolSize = 1000); // Default pool size
    void Shutdown();

    void Emit(const ParticleProperties& properties);
    void Update(float deltaTime);

    // Enhanced rendering with batching for better performance
    void Render(class Renderer& renderer);

    // Spark effects for material interactions
    void CreateSparks(Vector2 position, int count);
    void CreateExplosion(Vector2 position, float intensity, int particleCount);
    void CreateTrail(Vector2 start, Vector2 end, const Vector3& color, int segments);

    // Physics effects
    void SetGravity(float gravity) { m_gravity = gravity; }
    float GetGravity() const { return m_gravity; }
    
    // Performance monitoring
    size_t GetActiveParticleCount() const;
    size_t GetMaxParticles() const { return m_poolSize; }

private:
    Particle* FindInactiveParticle();
    void UpdatePhysics(Particle& particle, float deltaTime);

    std::vector<Particle> m_particlePool;
    size_t m_poolSize;
    size_t m_currentIndex = 0; // For round-robin inactive particle search

    // Physics properties
    float m_gravity = 98.0f; // Pixels/second^2
    static constexpr float AIR_RESISTANCE = 0.99f; // Velocity damping factor
};

} // namespace BGE
