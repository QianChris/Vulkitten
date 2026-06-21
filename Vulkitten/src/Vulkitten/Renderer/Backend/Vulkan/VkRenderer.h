#pragma once

#include "Vulkitten/Renderer/IRenderer.h"
#include "Vulkitten/Renderer/RenderGraph/RenderGraph.h"
#include "Vulkitten/Renderer/FrameContext.h"
#include "Vulkitten/Renderer/Shader.h"

namespace Vulkitten {

class VulkanInstance;
class VulkanDevice;
class VkSwapchain;
class VkGpuResourceManager;

// ============================================================
// VkRenderer — Vulkan implementation of IRenderer.
//
// Owns all Vulkan backend dependencies: VulkanInstance,
// VulkanDevice, VkSwapchain, VkGpuResourceManager, and
// ShaderLibrary. Created by Application via RendererConfig.
//
// Lifecycle:
//   Init → BeginFrame → Execute → EndFrame → ... → Shutdown
// ============================================================
class VkRenderer : public IRenderer
{
public:
    explicit VkRenderer(const RendererConfig& config);
    ~VkRenderer();

    // ---- IRenderer Lifecycle ----
    void Init() override;
    void Shutdown() override;

    // ---- IRenderer Per-Frame ----
    void BeginFrame() override;
    void Execute() override;
    void EndFrame() override;

    // ---- IRenderer Window Events ----
    void OnWindowResize(uint32_t width, uint32_t height) override;

    // ---- IRenderer Subsystem Access ----
    IDevice&             GetDevice() override;
    IGpuResourceManager& GetResourceManager() override;
    RenderGraph*         GetRenderGraph() override     { return m_RenderGraph; }
    ShaderLibrary&       GetShaderLibrary() override   { return m_ShaderLibrary; }

    // Vulkan command recording
    void BeginCommandBuffer(uint32_t imageIndex);
    void EndCommandBuffer();

private:
    const RendererConfig& m_Config;

    Scope<VulkanInstance>        m_Instance;
    Scope<VulkanDevice>          m_Device;
    Scope<VkSwapchain>            m_Swapchain;
    Scope<VkGpuResourceManager>   m_Resources;
    Scope<FrameContext>           m_FrameContext;
    ShaderLibrary                 m_ShaderLibrary;

    void* m_CommandPool = nullptr;       // VkCommandPool
    void* m_CommandBuffer = nullptr;     // VkCommandBuffer (per-frame)

    RenderGraph* m_RenderGraph = nullptr;
};

} // namespace Vulkitten
