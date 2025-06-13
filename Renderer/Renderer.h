#pragma once

#include <memory>

namespace BGE {

class Window;
class SimulationWorld;
class LightingSystem;

class Renderer {
public:
    Renderer();
    ~Renderer();
    
    bool Initialize(Window* window);
    void Shutdown();
    
    void BeginFrame();
    void EndFrame();
    
    void RenderWorld(SimulationWorld* world);
    
    LightingSystem* GetLightingSystem() const { return m_lightingSystem.get(); }
    
    // Render statistics
    uint64_t GetFrameCount() const { return m_frameCount; }
    float GetLastFrameTime() const { return m_lastFrameTime; }

private:
    void InitializeGraphicsAPI();
    void CreateRenderTargets();
    void SetupDefaultPipeline();
    void CreateSimulationTexture(uint32_t width, uint32_t height);
    void UpdateSimulationTexture(const uint8_t* pixelData, uint32_t width, uint32_t height);
    void RenderFullscreenQuad();
    
    Window* m_window = nullptr;
    std::unique_ptr<LightingSystem> m_lightingSystem;
    
    // OpenGL resources
    uint32_t m_simulationTexture = 0;
    uint32_t m_quadVAO = 0;
    uint32_t m_quadVBO = 0;
    uint32_t m_shaderProgram = 0;
    
    uint64_t m_frameCount = 0;
    float m_lastFrameTime = 0.0f;
    
    bool m_initialized = false;
};

} // namespace BGE