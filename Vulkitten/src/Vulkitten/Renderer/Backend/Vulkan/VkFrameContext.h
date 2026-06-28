#pragma once

#include "Vulkitten/Renderer/FrameContext.h"

namespace Vulkitten {

class VulkanDevice;

// ============================================================
// VkFrameContext - Vulkan per-frame state.
//
// Extends FrameContext with real Vulkan synchronization objects:
// VkCommandPool, VkFence (in-flight), VkSemaphore pairs
// (image-available + render-finished), and per-frame
// VkCommandBuffer.
// ============================================================
class VkFrameContext : public FrameContext
{
public:
    VkFrameContext() = default;
    ~VkFrameContext() = default;

    // Initialize Vulkan per-frame resources from device.
    void Init(VulkanDevice& device, uint32_t maxFramesInFlight);

    // Reset for a new frame (reset command pool, etc.).
    void Reset();

private:
    void* m_CommandPools[3] = {};    // VkCommandPool per frame-in-flight
};

} // namespace Vulkitten
