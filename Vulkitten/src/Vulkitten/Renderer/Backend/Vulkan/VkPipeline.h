#pragma once

#include "Vulkitten/Core/Core.h"

namespace Vulkitten {

class VulkanDevice;

// ============================================================
// VkPipeline - Vulkan graphics pipeline wrapper.
//
// Creates VkPipeline with layout, descriptor set layout, shader
// stages, vertex input state, and render pass compatibility.
// Supports the SpriteRenderPass BatchVertex layout.
// ============================================================
class VkPipeline
{
public:
    VkPipeline() = default;
    ~VkPipeline() = default;

    // Create a graphics pipeline with the given shader modules
    // and vertex layout description.
    bool Create(VulkanDevice& device, void* vertShaderModule, void* fragShaderModule,
                void* renderPass, const void* vertexLayoutDesc);

    void* GetNativePipeline()     const { return m_Pipeline; }
    void* GetNativePipelineLayout() const { return m_PipelineLayout; }

private:
    void* m_Pipeline = nullptr;        // VkPipeline
    void* m_PipelineLayout = nullptr;  // VkPipelineLayout
};

} // namespace Vulkitten
