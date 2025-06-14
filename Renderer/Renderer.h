#pragma once

#include <memory>
#include <cstdint>
#include "../Core/Math/Matrix4.h" // For BGE::Matrix4
#include "../Core/Platform/Window.h" // For BGE::Window

#include "PixelCamera.h" // Added include for PixelCamera
#include "ParticleSystem.h" // Added include for ParticleSystem

// Forward declarations
namespace BGE {
    class SimulationWorld;
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
    
    // Viewport management
    void SetSimulationViewport(int x, int y, int width, int height);

    // World rendering
    void RenderWorld(class SimulationWorld* world);

    // Particle system methods
    void RenderParticles();

    // Primitive drawing methods (used by ParticleSystem for now)
    void DrawPrimitivePixel(int x, int y, const Vector3& color);

    // Texture management for asset pipeline
    uint32_t CreateTexture(int width, int height, int channels, const void* data);
    void UpdateTexture(uint32_t textureId, int width, int height, int channels, const void* data);
    void DeleteTexture(uint32_t textureId);

    PixelCamera* GetPixelCamera() const { return m_pixelCamera.get(); }
    Window* GetWindow() const { return m_window; }

private:
    Window* m_window = nullptr;
    std::unique_ptr<PixelCamera> m_pixelCamera;
    uint32_t m_nextTextureId = 1; // Simple way to generate unique IDs for placeholder

    // Simulation viewport settings
    int m_simViewportX = 0;
    int m_simViewportY = 0;
    int m_simViewportWidth = 512;
    int m_simViewportHeight = 512;

    // Placeholder for actual rendering context or device
    void* m_renderContext = nullptr;
};

} // namespace BGE