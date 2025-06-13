#pragma once

#include "../RenderDevice.h"

namespace BGE {

class VulkanRenderer : public RenderDevice {
public:
    VulkanRenderer();
    ~VulkanRenderer() override;
    
    bool Initialize() override;
    void Shutdown() override;
    
    void BeginFrame() override;
    void EndFrame() override;
    
    // Vulkan-specific methods
    bool CreateInstance();
    bool CreateDevice();
    bool CreateSwapchain();
    
private:
    // Vulkan handles (using void* to avoid including Vulkan headers)
    void* m_instance = nullptr;
    void* m_device = nullptr;
    void* m_swapchain = nullptr;
    void* m_commandPool = nullptr;
    
    bool m_initialized = false;
};

} // namespace BGE