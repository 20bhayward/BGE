#pragma once

namespace BGE {

class RenderDevice {
public:
    RenderDevice() = default;
    virtual ~RenderDevice() = default;
    
    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;
    
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    
    // TODO: Add rendering interface
};

} // namespace BGE