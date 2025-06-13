#pragma once

#include <memory>
#include "../Core/Math/Matrix4.h" // For BGE::Matrix4
#include "../Core/Platform/Window.h" // For BGE::Window

#include "PixelCamera.h" // Added include for PixelCamera
#include "ParticleSystem.h" // Added include for ParticleSystem

// Forward declarations
namespace BGE {
    // class PixelCamera; // No longer needed due to include
    // class ParticleSystem; // No longer needed due to include
}

namespace BGE {

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool Initialize(Window* window);
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    // Temporary: until world rendering is fully handled by this Renderer
    void RenderWorld(class SimulationWorld* world);

    // Particle system methods
    // void UpdateParticles(float deltaTime); // Will be called by Engine directly
    void RenderParticles();

    // Primitive drawing methods (used by ParticleSystem for now)
    void DrawPrimitivePixel(int x, int y, const Vector3& color);
    // void DrawPrimitiveQuad(const Vector2& position, const Vector2& size, const Vector3& color);

    // Matrix4 GetViewMatrix() const; // This might be specific to PixelCamera

    PixelCamera* GetPixelCamera() const { return m_pixelCamera.get(); } // Ensure this is uncommented
    // ParticleSystem* GetParticleSystem() const; // Removed

private:
    Window* m_window = nullptr;
    std::unique_ptr<PixelCamera> m_pixelCamera; // Ensure this is uncommented
    // std::unique_ptr<ParticleSystem> m_particleSystem; // Removed

    // Placeholder for actual rendering context or device
    void* m_renderContext = nullptr;
};

} // namespace BGE
