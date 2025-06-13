#pragma once

#include "GraphicsAPI.h"
#include <string>

namespace BGE {

struct PipelineState {
    // Vertex input
    bool enableDepthTest = true;
    bool enableBlending = false;
    
    // Rasterizer state
    bool enableWireframe = false;
    bool enableCulling = true;
    
    // Blend state
    bool enableAlphaBlending = false;
    
    // Shader paths
    std::string vertexShaderPath;
    std::string fragmentShaderPath;
    std::string computeShaderPath;
};

class Pipeline {
public:
    Pipeline();
    virtual ~Pipeline();
    
    virtual bool Create(const PipelineState& state) = 0;
    virtual void Bind() = 0;
    virtual void Unbind() = 0;
    virtual void Destroy() = 0;
    
    bool IsValid() const { return m_valid; }
    
protected:
    bool m_valid = false;
};

} // namespace BGE