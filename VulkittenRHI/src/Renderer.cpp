#include "rhi/Renderer.hpp"
#include "rhi/IRenderDevice.hpp"
#include "rhi/ICommandBuffer.hpp"
#include "rhi/RenderCommandList.hpp"
#include "rhi/ResourceManager.hpp"
#include "rhi/ResourceDescs.hpp"

#include "gl/GLDevice.hpp"

#ifdef RHI_HAS_VULKAN
#include "vk/VKDevice.hpp"
#endif

#include <stdexcept>
#include <cstdio>

namespace rhi {

// ============================================================
// Renderer::Impl — PIMPL
// ============================================================

struct Renderer::Impl
{
    RendererConfig         Config;
    std::unique_ptr<ResourceManager> Resources;
    std::unique_ptr<IRenderDevice> Device;
    std::unique_ptr<ICommandBuffer> CurrentCmd;
    std::unique_ptr<RenderCommandList> CurrentList;
    FrameContext           CurrentCtx;
    bool                   FrameInProgress = false;
    uint64_t               FrameCount = 0;

    // Default swapchain resources (created once on first access)
    RenderPassHandle       DefaultRenderPass;
    FramebufferHandle      DefaultFramebuffer;
};

// ============================================================
// Factory
// ============================================================

std::unique_ptr<Renderer> Renderer::Create(const RendererConfig& config)
{
    if (!config.Surface)
        throw std::runtime_error("Renderer::Create: no surface provided");

    auto renderer = std::unique_ptr<Renderer>(new Renderer());
    renderer->m_Impl = std::make_unique<Impl>();
    renderer->m_Impl->Config = config;

    // Create ResourceManager first (device needs it)
    renderer->m_Impl->Resources = std::make_unique<ResourceManager>();

    // Create backend device
    switch (config.Backend)
    {
        case BackendType::OpenGL:
            renderer->m_Impl->Device = std::make_unique<GLDevice>(
                config.Surface, *renderer->m_Impl->Resources);
            break;

        case BackendType::Vulkan:
#ifdef RHI_HAS_VULKAN
            renderer->m_Impl->Device = std::make_unique<VKDevice>(
                config.Surface, *renderer->m_Impl->Resources);
#else
            throw std::runtime_error("Renderer::Create: Vulkan backend not available "
                                     "(rebuild with Vulkan SDK)");
#endif
            break;

        default:
            throw std::runtime_error("Renderer::Create: unknown backend type");
    }

    renderer->m_Impl->Device->Init();

    // Create default swapchain render pass and framebuffer (once)
    {
        auto surfDesc = config.Surface->GetDesc();

        // RenderPass: single color attachment, clear + store
        RenderPassDesc rpDesc;
        AttachmentDesc colorAttach;
        colorAttach.Format = Format::BGRA8_UNORM;
        colorAttach.LoadOp = LoadOp::Clear;
        colorAttach.StoreOp = StoreOp::Store;
        colorAttach.InitialLayout = ImageLayout::Undefined;
        colorAttach.FinalLayout = ImageLayout::PresentSrc;
        rpDesc.ColorAttachments.push_back(colorAttach);

        SubpassDesc subpass;
        subpass.ColorAttachments.push_back(0);
        rpDesc.Subpasses.push_back(subpass);

        renderer->m_Impl->DefaultRenderPass =
            renderer->m_Impl->Device->CreateRenderPass(rpDesc);

        // Framebuffer
        FramebufferDesc fbDesc;
        fbDesc.RenderPass = renderer->m_Impl->DefaultRenderPass;
        fbDesc.Width = surfDesc.Width;
        fbDesc.Height = surfDesc.Height;

        renderer->m_Impl->DefaultFramebuffer =
            renderer->m_Impl->Device->CreateFramebuffer(fbDesc);
    }

    return renderer;
}

Renderer::~Renderer()
{
    if (m_Impl)
    {
        if (m_Impl->Device)
        {
            m_Impl->Device->WaitIdle();
        }

        // IMPORTANT: Destroy all GPU resources BEFORE shutting down the device.
        // ResourceManager::DestroyAll() triggers RAII destructors which call
        // vkDestroyBuffer/vkDestroyPipeline/etc. These must complete before
        // vkDestroyDevice is called.
        if (m_Impl->Resources)
        {
            m_Impl->Resources->DestroyAll();
        }

        if (m_Impl->Device)
        {
            m_Impl->Device->Shutdown();
        }
    }
}

// ============================================================
// Frame Lifecycle
// ============================================================

void Renderer::BeginFrame()
{
    if (!m_Impl || !m_Impl->Device)
        return;

    m_Impl->CurrentCtx = m_Impl->Device->BeginFrame();

    // Create a command buffer for this frame
    m_Impl->CurrentCmd = m_Impl->Device->CreateCommandBuffer(m_Impl->CurrentCtx);

    // Create the thin adapter list
    if (m_Impl->CurrentCmd)
    {
        m_Impl->CurrentList = std::make_unique<RenderCommandList>(
            *m_Impl->Device, *m_Impl->Resources, *m_Impl->CurrentCmd);
        m_Impl->CurrentCmd->Begin();
    }

    m_Impl->FrameInProgress = true;
}

void Renderer::EndFrame()
{
    if (!m_Impl || !m_Impl->Device)
        return;

    if (m_Impl->CurrentCmd)
    {
        m_Impl->CurrentCmd->End();
    }

    m_Impl->CurrentList.reset();
    m_Impl->CurrentCmd.reset();

    m_Impl->Device->EndFrame(m_Impl->CurrentCtx);

    m_Impl->FrameInProgress = false;
    m_Impl->FrameCount++;
}

bool Renderer::IsFrameInProgress() const
{
    return m_Impl && m_Impl->FrameInProgress;
}

// ============================================================
// Accessors
// ============================================================

IRenderDevice& Renderer::GetDevice()
{
    return *m_Impl->Device;
}

ResourceManager& Renderer::GetResources()
{
    return *m_Impl->Resources;
}

RenderCommandList& Renderer::GetCommandList()
{
    return *m_Impl->CurrentList;
}

ICommandBuffer& Renderer::GetCommandBuffer()
{
    return *m_Impl->CurrentCmd;
}

BackendType Renderer::GetBackendType() const
{
    return m_Impl->Config.Backend;
}

uint32_t Renderer::GetFrameIndex() const
{
    return m_Impl->CurrentCtx.FrameIndex;
}

uint64_t Renderer::GetFrameCount() const
{
    return m_Impl->FrameCount;
}

uint32_t Renderer::GetSwapchainIndex() const
{
    return m_Impl->CurrentCtx.SwapchainIndex;
}

RenderPassHandle Renderer::GetDefaultRenderPass()
{
    return m_Impl->DefaultRenderPass;
}

FramebufferHandle Renderer::GetDefaultFramebuffer()
{
    return m_Impl->DefaultFramebuffer;
}

// ============================================================
// Window Events
// ============================================================

void Renderer::OnResize(uint32_t width, uint32_t height)
{
    if (m_Impl && m_Impl->Device)
        m_Impl->Device->OnResize(width, height);
}

void Renderer::WaitIdle()
{
    if (m_Impl && m_Impl->Device)
        m_Impl->Device->WaitIdle();
}

} // namespace rhi
