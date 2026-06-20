#pragma once

#include "Vulkitten/Renderer/IRenderer.h"
#include "Vulkitten/Renderer/RenderGraph/RenderGraph.h"
#include "Vulkitten/Renderer/FrameContext.h"

namespace Vulkitten {

class VulkanInstance;
class VulkanDevice;
class VkSwapchain;
class VkGpuResourceManager;
class IWindow;

// ============================================================
// VkRenderer — Vulkan implementation of IRenderer.
//
// The top-level Vulkan backend. Created with a RendererConfig
// containing the Vulkan device, resource manager, and render
// graph. Manages the frame lifecycle, swapchain, and pass
// execution via VkRenderContext.
//
// Lifecycle:
//   Init → BeginFrame → Execute → EndFrame → ... → Shutdown
// ============================================================
class VkRenderer : public IRenderer
{
public:
    VkRenderer(const RendererConfig& config, VulkanInstance& instance,
               IWindow& window);
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
    RenderGraph*         GetRenderGraph() override;

private:
    const RendererConfig& m_Config;
    VulkanInstance&       m_VulkanInstance;
    IWindow&              m_Window;

    Scope<VulkanDevice>              m_Device;
    Scope<VkSwapchain>            m_Swapchain;
    Scope<VkGpuResourceManager>   m_Resources;
    Scope<FrameContext>           m_FrameContext;

    RenderGraph* m_RenderGraph = nullptr;
};

} // namespace Vulkitten
