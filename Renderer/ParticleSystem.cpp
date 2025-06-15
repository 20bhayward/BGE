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
            UpdatePhysics(m_particlePool[i], deltaTime);
        }
    }
}

void ParticleSystem::UpdatePhysics(Particle& particle, float deltaTime) {
    // Apply gravity
    particle.velocity.y += m_gravity * deltaTime;

    // Apply air resistance
    particle.velocity.x *= AIR_RESISTANCE;
    particle.velocity.y *= AIR_RESISTANCE;

    // Update position
    particle.position += particle.velocity * deltaTime;

    // Decrease lifetime
    particle.lifetime -= deltaTime;

    // Fade color based on lifetime (visual enhancement)
    if (particle.lifetime < 0.3f) {
        float fadeAlpha = particle.lifetime / 0.3f;
        particle.color = particle.color * fadeAlpha;
    }

    // Deactivate if lifetime is over
    if (particle.lifetime <= 0.0f) {
        particle.isActive = false;
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

void ParticleSystem::Render(Renderer& renderer) {
    // Batch render all active particles for better performance
    size_t activeCount = 0;
    for (size_t i = 0; i < m_poolSize; ++i) {
        if (m_particlePool[i].isActive) {
            const Particle& p = m_particlePool[i];
            renderer.DrawPrimitivePixel(static_cast<int>(p.position.x),
                                        static_cast<int>(p.position.y),
                                        p.color);
            activeCount++;
        }
    }
    
    // Log performance metrics periodically
    static int frameCounter = 0;
    if (++frameCounter % 300 == 0) { // Every 5 seconds at 60 FPS
        BGE_LOG_DEBUG("ParticleSystem", "Rendered " + std::to_string(activeCount) + 
                      "/" + std::to_string(m_poolSize) + " particles");
    }
}

void ParticleSystem::CreateExplosion(Vector2 position, float intensity, int particleCount) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> angle_dist(0.0f, 2.0f * 3.14159f); // Full circle
    std::uniform_real_distribution<> speed_dist(intensity * 0.5f, intensity * 1.5f);
    std::uniform_real_distribution<> life_dist(0.8f, 2.0f);
    
    for (int i = 0; i < particleCount; ++i) {
        float angle = static_cast<float>(angle_dist(gen));
        float speed = static_cast<float>(speed_dist(gen));
        
        ParticleProperties props;
        props.position = position;
        props.velocity = Vector2(std::cos(angle) * speed, std::sin(angle) * speed);
        props.lifetime = static_cast<float>(life_dist(gen));
        
        // Hot explosion colors (red/orange/yellow)
        float colorChoice = static_cast<float>(std::uniform_real_distribution<>(0.0, 1.0)(gen));
        if (colorChoice < 0.33f) {
            props.color = Vector3(1.0f, 0.2f, 0.0f); // Red
        } else if (colorChoice < 0.66f) {
            props.color = Vector3(1.0f, 0.6f, 0.0f); // Orange
        } else {
            props.color = Vector3(1.0f, 1.0f, 0.2f); // Yellow
        }
        
        Emit(props);
    }
}

void ParticleSystem::CreateTrail(Vector2 start, Vector2 end, const Vector3& color, int segments) {
    if (segments <= 0) return;
    
    Vector2 direction = end - start;
    float segmentLength = 1.0f / static_cast<float>(segments);
    
    for (int i = 0; i < segments; ++i) {
        float t = static_cast<float>(i) * segmentLength;
        Vector2 position = start + direction * t;
        
        ParticleProperties props;
        props.position = position;
        props.velocity = Vector2(0.0f, 0.0f); // Stationary trail particles
        props.color = color;
        props.lifetime = 0.5f + (1.0f - t) * 0.5f; // Longer lifetime at start
        
        Emit(props);
    }
}

size_t ParticleSystem::GetActiveParticleCount() const {
    size_t count = 0;
    for (const auto& particle : m_particlePool) {
        if (particle.isActive) {
            count++;
        }
    }
    return count;
}

} // namespace BGE
