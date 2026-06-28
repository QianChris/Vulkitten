#include "vktpch.h"
#include "VkPipeline.h"

#include "VulkanDevice.h"

namespace Vulkitten {

bool VkPipeline::Create(VulkanDevice& /*device*/, void* /*vertShaderModule*/,
                        void* /*fragShaderModule*/, void* /*renderPass*/,
                        const void* /*vertexLayoutDesc*/)
{
    // Stub: real implementation creates VkPipelineLayout,
    // VkDescriptorSetLayout, shader stages, vertex input state,
    // and calls vkCreateGraphicsPipelines
    return true;
}

} // namespace Vulkitten
