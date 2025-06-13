#include "ParticleSystem.h"
#include "../Core/Logger.h"
#include <cstdlib> // For rand() if needed for CreateSparks later, or use C++ random
#include <algorithm> // For std::min/max if needed
#include <random>    // For std::mt19937 and distributions
#include "../Renderer/Renderer.h" // Include Renderer.h for Renderer class definition

namespace BGE {

ParticleSystem::ParticleSystem() : m_poolSize(0) {
    // Constructor
}

ParticleSystem::~ParticleSystem() {
    // Destructor
}

bool ParticleSystem::Initialize(size_t poolSize) {
    m_poolSize = poolSize;
    m_particlePool.resize(m_poolSize);
    for (size_t i = 0; i < m_poolSize; ++i) {
        m_particlePool[i].isActive = false;
    }
    m_currentIndex = 0;
    BGE_LOG_INFO("ParticleSystem", "Initialized with pool size: " + std::to_string(m_poolSize));
    return true;
}

void ParticleSystem::Shutdown() {
    m_particlePool.clear();
    BGE_LOG_INFO("ParticleSystem", "Shutdown complete.");
}

Particle* ParticleSystem::FindInactiveParticle() {
    // Round-robin search for an inactive particle
    for (size_t i = 0; i < m_poolSize; ++i) {
        size_t index = (m_currentIndex + i) % m_poolSize;
        if (!m_particlePool[index].isActive) {
            m_currentIndex = (index + 1) % m_poolSize;
            return &m_particlePool[index];
        }
    }
    // If no inactive particle is found (pool is full), overwrite the current one (oldest)
    // This is a common strategy, alternatively, emission could fail.
    Particle* p = &m_particlePool[m_currentIndex];
    m_currentIndex = (m_currentIndex + 1) % m_poolSize;
    // BGE_LOG_WARN("ParticleSystem", "Particle pool full. Overwriting oldest particle.");
    return p;
}

void ParticleSystem::Emit(const ParticleProperties& properties) {
    Particle* particle = FindInactiveParticle();
    if (!particle) {
        // Should not happen if FindInactiveParticle always returns one (even if overwriting)
        // BGE_LOG_WARN("ParticleSystem", "Could not find inactive particle to emit.");
        return;
    }

    particle->position = properties.position;
    particle->velocity = properties.velocity;
    particle->color = properties.color;
    particle->lifetime = properties.lifetime;
    particle->isActive = true;
    // particle->size = properties.size; // Future
    // particle->rotation = properties.rotation; // Future
}

void ParticleSystem::Update(float deltaTime) {
    for (size_t i = 0; i < m_poolSize; ++i) {
        if (m_particlePool[i].isActive) {
            Particle& p = m_particlePool[i];

            // Apply gravity
            p.velocity.y += DEFAULT_GRAVITY * deltaTime;

            // Update position
            p.position += p.velocity * deltaTime;

            // Decrease lifetime
            p.lifetime -= deltaTime;

            // Deactivate if lifetime is over
            if (p.lifetime <= 0.0f) {
                p.isActive = false;
            }
        }
    }
}

void ParticleSystem::CreateSparks(Vector2 position, int count) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> vel_dist(-50.0f, 50.0f); // Velocity range for x
    std::uniform_real_distribution<> vel_y_dist(-100.0f, -20.0f); // Velocity range for y (upwards)
    std::uniform_real_distribution<> life_dist(0.5f, 1.5f);    // Lifetime range
    std::uniform_int_distribution<> color_choice(0, 1);         // For choosing between two colors

    for (int i = 0; i < count; ++i) {
        ParticleProperties props;
        props.position = position;
        props.velocity = Vector2(static_cast<float>(vel_dist(gen)), static_cast<float>(vel_y_dist(gen)));
        props.lifetime = static_cast<float>(life_dist(gen));

        if (color_choice(gen) == 0) {
            props.color = Vector3(1.0f, 0.5f, 0.0f); // Orange
        } else {
            props.color = Vector3(1.0f, 1.0f, 0.0f); // Yellow
        }

        Emit(props);
    }
}

void ParticleSystem::Render(Renderer& renderer) { // Accept Renderer reference
    // BGE_LOG_TRACE("ParticleSystem", "Render called.");
    for (size_t i = 0; i < m_poolSize; ++i) {
        if (m_particlePool[i].isActive) {
            const Particle& p = m_particlePool[i];
            // Call a new method on Renderer to draw a small quad or pixel
            // This method needs to be added to Renderer.
            renderer.DrawPrimitivePixel(static_cast<int>(p.position.x),
                                        static_cast<int>(p.position.y),
                                        p.color);
            // Or for a 2x2 quad:
            // renderer.DrawPrimitiveQuad(p.position, Vector2(2.0f, 2.0f), p.color);
        }
    }
}

} // namespace BGE
