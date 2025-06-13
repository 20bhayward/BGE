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

    // Render method might take a Renderer reference or a specific drawing context
    void Render(class Renderer& renderer); // Forward declare Renderer or include its header

    // For CreateSparks, to be implemented in a later step
    void CreateSparks(Vector2 position, int count);

private:
    Particle* FindInactiveParticle();

    std::vector<Particle> m_particlePool;
    size_t m_poolSize;
    size_t m_currentIndex = 0; // For round-robin inactive particle search

    // Constants
    static constexpr float DEFAULT_GRAVITY = 98.0f; // Pixels/second^2, adjust as needed for game scale
};

} // namespace BGE
