#pragma once

#include <cstdint>

namespace BGE {

enum class GraphicsAPI {
    None,
    OpenGL,
    Vulkan,
    DirectX11,
    DirectX12
};

enum class BufferType {
    Vertex,
    Index,
    Uniform,
    Storage
};

enum class TextureFormat {
    RGB8,
    RGBA8,
    RGB16F,
    RGBA16F,
    RGB32F,
    RGBA32F,
    Depth24Stencil8
};

enum class PrimitiveType {
    Points,
    Lines,
    Triangles,
    TriangleStrip
};

struct GraphicsCapabilities {
    bool supportsCompute = false;
    bool supportsRaytracing = false;
    bool supportsGeometryShaders = false;
    bool supportsTessellation = false;
    uint32_t maxTextureSize = 0;
    uint32_t maxTextureUnits = 0;
    uint32_t maxVertexAttributes = 0;
    uint32_t maxUniformBufferBindings = 0;
};

class GraphicsContext {
public:
    virtual ~GraphicsContext() = default;
    
    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;
    
    virtual void MakeCurrent() = 0;
    virtual void SwapBuffers() = 0;
    
    virtual GraphicsAPI GetAPI() const = 0;
    virtual GraphicsCapabilities GetCapabilities() const = 0;
    
    virtual void SetViewport(int x, int y, int width, int height) = 0;
    virtual void Clear(float r, float g, float b, float a) = 0;
    virtual void ClearDepth(float depth) = 0;
};

} // namespace BGE