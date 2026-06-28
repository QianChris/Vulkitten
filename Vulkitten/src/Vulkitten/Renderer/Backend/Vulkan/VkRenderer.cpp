#include "vktpch.h"
#include "VkRenderer.h"

#ifdef VKT_HAS_VULKAN
#include <vulkan/vulkan.h>
#endif

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

    // Create swapchain (requires VulkanInstance + IWindow)
    if (m_Config.Window)
    {
        m_Swapchain = CreateScope<VkSwapchain>(*m_Device, *m_Instance, *m_Config.Window);
        auto surfaceDesc = m_Config.Window->GetSurfaceDesc();
        m_Swapchain->Create(surfaceDesc.Width, surfaceDesc.Height);
    }

    // Set up RenderGraph
    m_RenderGraph = new RenderGraph();

#ifdef VKT_HAS_VULKAN
    // Create command pool
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = static_cast<VulkanDevice&>(*m_Device).GetGraphicsQueueFamily();
    VkCommandPool cmdPool;
    vkCreateCommandPool(static_cast<VkDevice>(m_Device->GetNativeDevice()), &poolInfo, nullptr, &cmdPool);
    m_CommandPool = cmdPool;

    // Allocate command buffer
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = cmdPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer cmdBuf;
    vkAllocateCommandBuffers(static_cast<VkDevice>(m_Device->GetNativeDevice()), &allocInfo, &cmdBuf);
    m_CommandBuffer = cmdBuf;
#endif

    VKT_CORE_INFO("VkRenderer: Vulkan backend initialized");
}

void VkRenderer::Shutdown()
{
    VKT_PROFILE_FUNCTION();

#ifdef VKT_HAS_VULKAN
    auto vkDevice = static_cast<VkDevice>(m_Device ? m_Device->GetNativeDevice() : nullptr);
    if (m_CommandPool && vkDevice)
    {
        vkDestroyCommandPool(vkDevice, static_cast<VkCommandPool>(m_CommandPool), nullptr);
        m_CommandPool = nullptr;
    }
#endif

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
        uint32_t imageIndex = 0;
        m_Swapchain->AcquireNextImage(imageIndex, 0);
        m_FrameContext->SwapchainIndex = imageIndex;

        // Record clear commands
        BeginCommandBuffer(imageIndex);
    }
}

void VkRenderer::BeginCommandBuffer(uint32_t imageIndex)
{
#ifdef VKT_HAS_VULKAN
    if (!m_CommandBuffer || !m_Swapchain) return;

    auto vkCmdBuf = static_cast<VkCommandBuffer>(m_CommandBuffer);
    auto vkRenderPass = static_cast<VkRenderPass>(m_Swapchain->GetRenderPass());
    auto vkFramebuffer = static_cast<VkFramebuffer>(m_Swapchain->GetFramebuffer(imageIndex));

    if (!vkRenderPass || !vkFramebuffer)
    {
        VKT_CORE_WARN("VkRenderer: RenderPass or Framebuffer is null - swapchain may not be fully created");
        return;
    }

    uint32_t w = m_Swapchain->GetWidth();
    uint32_t h = m_Swapchain->GetHeight();

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(vkCmdBuf, &beginInfo);

    VkClearValue clearColor = {{{0.1f, 0.1f, 0.15f, 1.0f}}};

    VkRenderPassBeginInfo rpBegin{};
    rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBegin.renderPass = vkRenderPass;
    rpBegin.framebuffer = vkFramebuffer;
    rpBegin.renderArea.extent = {w, h};
    rpBegin.clearValueCount = 1;
    rpBegin.pClearValues = &clearColor;
    vkCmdBeginRenderPass(vkCmdBuf, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdEndRenderPass(vkCmdBuf);
    vkEndCommandBuffer(vkCmdBuf);
#endif
}

void VkRenderer::Execute()
{
    if (m_RenderGraph)
        m_RenderGraph->Execute();
}

void VkRenderer::EndFrame()
{
    VKT_PROFILE_FUNCTION();

    // Submit to device (vkQueueSubmit + vkQueuePresentKHR)
    if (m_Device && m_FrameContext)
        m_Device->Submit(*m_FrameContext);

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
