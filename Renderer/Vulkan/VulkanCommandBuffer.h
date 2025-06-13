#pragma once

#include "../CommandBuffer.h"

namespace BGE {

class VulkanCommandBuffer : public CommandBuffer {
public:
    VulkanCommandBuffer();
    ~VulkanCommandBuffer();
    
    // TODO: Vulkan-specific command buffer implementation

private:
    void* m_commandBuffer = nullptr;
};

} // namespace BGE