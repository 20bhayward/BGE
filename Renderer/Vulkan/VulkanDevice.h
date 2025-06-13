#pragma once

namespace BGE {

class VulkanDevice {
public:
    VulkanDevice();
    ~VulkanDevice();
    
    bool Initialize();
    void Shutdown();
    
    // TODO: Add Vulkan device methods

private:
    void* m_device = nullptr;
    bool m_initialized = false;
};

} // namespace BGE