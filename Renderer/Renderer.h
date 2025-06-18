#pragma once

#include <memory>
#include <cstdint>
#include "../Core/Math/Matrix4.h" // For BGE::Matrix4
#include "../Core/Platform/Window.h" // For BGE::Window

#include "PixelCamera.h" // Added include for PixelCamera
#include "ParticleSystem.h" // Added include for ParticleSystem
#include "PostProcessor.h" // Added include for PostProcessor

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

    // Render-to-texture support for UI panels
    bool CreateGameFramebuffer(int width, int height);
    void DestroyGameFramebuffer();
    void BeginRenderToTexture();
    void EndRenderToTexture();
    uint32_t GetGameTextureId() const { return m_gameTextureId; }
    void GetGameTextureSize(int& width, int& height) const { width = m_gameTextureWidth; height = m_gameTextureHeight; }
    
    // Scene view framebuffer support
    bool CreateSceneFramebuffer(int width, int height);
    void DestroySceneFramebuffer();
    void BeginRenderToSceneTexture();
    void EndRenderToSceneTexture();
    uint32_t GetSceneTextureId() const { return m_sceneTextureId; }
    void GetSceneTextureSize(int& width, int& height) const { width = m_sceneTextureWidth; height = m_sceneTextureHeight; }

    PixelCamera* GetPixelCamera() const { return m_pixelCamera.get(); }
    PostProcessor* GetPostProcessor() const { return m_postProcessor.get(); }
    Window* GetWindow() const { return m_window; }

private:
    Window* m_window = nullptr;
    std::unique_ptr<PixelCamera> m_pixelCamera;
    std::unique_ptr<PostProcessor> m_postProcessor;
    uint32_t m_nextTextureId = 1; // Simple way to generate unique IDs for placeholder

    // Simulation viewport settings
    int m_simViewportX = 0;
    int m_simViewportY = 0;
    int m_simViewportWidth = 512;
    int m_simViewportHeight = 512;

    // Render-to-texture framebuffer for game panels
    uint32_t m_gameFramebuffer = 0;
    uint32_t m_gameTextureId = 0;
    uint32_t m_gameDepthBuffer = 0;
    int m_gameTextureWidth = 512;
    int m_gameTextureHeight = 512;
    bool m_renderingToTexture = false;
    
    // Render-to-texture framebuffer for scene view
    uint32_t m_sceneFramebuffer = 0;
    uint32_t m_sceneTextureId = 0;
    uint32_t m_sceneDepthBuffer = 0;
    int m_sceneTextureWidth = 512;
    int m_sceneTextureHeight = 512;
    bool m_renderingToSceneTexture = false;

    // Placeholder for actual rendering context or device
    void* m_renderContext = nullptr;
};

} // namespace BGE