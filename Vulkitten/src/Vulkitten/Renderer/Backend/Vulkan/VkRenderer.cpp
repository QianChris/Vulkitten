#include "vktpch.h"
#include "VkRenderer.h"

#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "VkSwapchain.h"
#include "VkGpuResourceManager.h"
#include "Vulkitten/Core/IWindow.h"
#include "Vulkitten/Core/ISurface.h"

#include "Vulkitten/Perf/Instrumentor.h"

namespace Vulkitten {

VkRenderer::VkRenderer(const RendererConfig& config, VulkanInstance& instance,
                       IWindow& window)
    : m_Config(config)
    , m_VulkanInstance(instance)
    , m_Window(window)
{
}

VkRenderer::~VkRenderer()
{
    Shutdown();
}

void VkRenderer::Init()
{
    VKT_PROFILE_FUNCTION();

    // Create Vulkan device
    m_Device = CreateScope<VulkanDevice>(m_VulkanInstance);
    m_Device->Init();

    // Create GPU resource manager
    m_Resources = CreateScope<VkGpuResourceManager>(*m_Device);

    // Create swapchain
    m_Swapchain = CreateScope<VkSwapchain>(*m_Device, m_Window);
    auto surfaceDesc = m_Window.GetSurfaceDesc();
    m_Swapchain->Create(surfaceDesc.Width, surfaceDesc.Height);

    // Set up RenderGraph (use config's graph or create own)
    m_RenderGraph = m_Config.Graph;
    if (!m_RenderGraph)
    {
        m_RenderGraph = new RenderGraph();
    }

    VKT_CORE_INFO("VkRenderer: Initialized");
}

void VkRenderer::Shutdown()
{
    VKT_PROFILE_FUNCTION();

    m_Swapchain.reset();
    m_Resources.reset();

    if (m_Device)
        m_Device->Shutdown();
    m_Device.reset();

    m_FrameContext.reset();
}

void VkRenderer::BeginFrame()
{
    VKT_PROFILE_FUNCTION();

    m_FrameContext = CreateScope<FrameContext>();

    // Acquire next swapchain image
    if (m_Swapchain)
    {
        m_Swapchain->AcquireNextImage(m_FrameContext->SwapchainIndex,
                                       m_FrameContext->ImageAvailableSemaphore);
    }
}

void VkRenderer::Execute()
{
    if (m_RenderGraph)
        m_RenderGraph->Execute();
}

void VkRenderer::EndFrame()
{
    VKT_PROFILE_FUNCTION();

    // Present
    if (m_Swapchain && m_FrameContext)
    {
        m_Swapchain->Present(m_FrameContext->SwapchainIndex,
                             m_FrameContext->RenderFinishedSemaphore);
    }

    m_FrameContext.reset();
}

void VkRenderer::OnWindowResize(uint32_t width, uint32_t height)
{
    VKT_PROFILE_FUNCTION();

    if (m_Swapchain)
    {
        m_Swapchain->Destroy();
        m_Swapchain->Create(width, height);
    }

    // Resize all registered framebuffers
    if (m_RenderGraph)
        m_RenderGraph->ResizeAllFramebuffers(width, height);
}

IDevice& VkRenderer::GetDevice()
{
    return *m_Device;
}

IGpuResourceManager& VkRenderer::GetResourceManager()
{
    return *m_Resources;
}

RenderGraph* VkRenderer::GetRenderGraph()
{
    return m_RenderGraph;
}

} // namespace Vulkitten
