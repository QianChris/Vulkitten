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

VkRenderer::VkRenderer(const RendererConfig& config)
    : m_Config(config)
{
}

VkRenderer::~VkRenderer()
{
    Shutdown();
}

void VkRenderer::Init()
{
    VKT_PROFILE_FUNCTION();

    // Register as global IRenderer instance FIRST
    s_Current = this;

    // Create Vulkan instance internally
    m_Instance = CreateScope<VulkanInstance>();
    m_Instance->Init(true);

    // Create Vulkan device
    m_Device = CreateScope<VulkanDevice>(*m_Instance);
    m_Device->Init();

    // Create GPU resource manager
    m_Resources = CreateScope<VkGpuResourceManager>(*m_Device);

    // Create swapchain (requires IWindow from config)
    if (m_Config.Window)
    {
        m_Swapchain = CreateScope<VkSwapchain>(*m_Device, *m_Config.Window);
        auto surfaceDesc = m_Config.Window->GetSurfaceDesc();
        m_Swapchain->Create(surfaceDesc.Width, surfaceDesc.Height);
    }

    // Set up RenderGraph
    m_RenderGraph = new RenderGraph();

    VKT_CORE_INFO("VkRenderer: Vulkan backend initialized");
}

void VkRenderer::Shutdown()
{
    VKT_PROFILE_FUNCTION();

    m_Swapchain.reset();
    m_Resources.reset();

    if (m_Device)
        m_Device->Shutdown();
    m_Device.reset();

    if (m_Instance)
        m_Instance->Shutdown();
    m_Instance.reset();

    delete m_RenderGraph;
    m_RenderGraph = nullptr;

    m_FrameContext.reset();
}

void VkRenderer::BeginFrame()
{
    VKT_PROFILE_FUNCTION();

    m_FrameContext = CreateScope<FrameContext>();

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

} // namespace Vulkitten
