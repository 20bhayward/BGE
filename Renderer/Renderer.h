#pragma once

#include <cstdint> // For uint32_t
#include <string>  // For std::string (logging)
// Forward declare Window if its full definition isn't needed here
// Based on Engine.cpp, Renderer::Initialize takes a Window*
namespace BGE { class Window; class SimulationWorld; }

namespace BGE {

class Renderer {
public:
    Renderer() = default;
    ~Renderer() = default;

    bool Initialize(Window* window); // Expected by Engine.cpp
    void Shutdown();

    void BeginFrame(); // Expected by Engine.cpp
    void EndFrame();   // Expected by Engine.cpp
    void RenderWorld(SimulationWorld* world); // Expected by Engine.cpp

    uint32_t CreateTexture(int width, int height, int channels, const void* data);
    void UpdateTexture(uint32_t textureId, int width, int height, int channels, const void* data);
    void DeleteTexture(uint32_t textureId); // Good practice to have a way to delete

private:
    uint32_t m_nextTextureId = 1; // Simple way to generate unique IDs for placeholder
};

} // namespace BGE
