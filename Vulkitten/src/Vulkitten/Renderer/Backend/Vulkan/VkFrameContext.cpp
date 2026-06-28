#include "vktpch.h"
#include "VkFrameContext.h"

#include "VulkanDevice.h"

namespace Vulkitten {

void VkFrameContext::Init(VulkanDevice& /*device*/, uint32_t /*maxFramesInFlight*/)
{
    // Stub: real implementation creates VkCommandPool per frame-in-flight
}

void VkFrameContext::Reset()
{
    // Stub: real implementation resets command pool and fences
}

} // namespace Vulkitten
