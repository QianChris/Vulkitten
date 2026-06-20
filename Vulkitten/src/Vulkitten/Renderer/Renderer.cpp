#include "vktpch.h"
#include "Renderer.h"
#include "Vulkitten/Renderer/Backend/OpenGL/OpenGLRendererAPI.h"

#include "Vulkitten/Renderer/Passes/PreparePass.h"
#include "Vulkitten/Renderer/Passes/GpuParticlePass.h"
#include "Vulkitten/Renderer/Passes/SpriteRenderPass.h"
#include "Vulkitten/Renderer/Passes/EndPass.h"

#include "Vulkitten/Renderer/GraphicsContext.h"
#include "Vulkitten/Core/Application.h"

#include "Vulkitten/Perf/Instrumentor.h"

namespace Vulkitten {

static uint32_t s_GlobalFrameIndex = 0;

Renderer::Renderer(IDevice* device, GpuResourceManager& resources, ShaderManager& shaders)
    : m_Device(device)
    , m_Resources(resources)
    , m_Shaders(shaders)
{
}

Renderer::~Renderer()
{
    delete m_RendererAPI;
    m_RendererAPI = nullptr;
}

void Renderer::Init()
{
    VKT_PROFILE_FUNCTION();

    m_RendererAPI = new OpenGLRendererAPI();
    m_RendererAPI->Init();

    m_RenderGraph = new RenderGraph();

    // Register default render passes (order matters)
    m_RenderGraph->AddPass(CreateRef<PreparePass>());       // Clear
    m_RenderGraph->AddPass(CreateRef<GpuParticlePass>());   // GPU particle update + render
    m_RenderGraph->AddPass(CreateRef<SpriteRenderPass>());  // 2D quad batch
    m_RenderGraph->AddPass(CreateRef<EndPass>());           // SwapBuffers (legacy, moving to EndFrame)
}

void Renderer::Shutdown()
{
    VKT_PROFILE_FUNCTION();

    delete m_RenderGraph;
    m_RenderGraph = nullptr;
}

void Renderer::BeginFrame()
{
    VKT_PROFILE_FUNCTION();

    m_FrameContext = CreateScope<FrameContext>();
    m_FrameContext->FrameIndex = s_GlobalFrameIndex++;
}

void Renderer::Execute()
{
    if (m_RenderGraph)
        m_RenderGraph->Execute();
}

void Renderer::EndFrame()
{
    VKT_PROFILE_FUNCTION();

    // Submit and present (SwapBuffers for OpenGL).
    // For OpenGL, this is just a swap — GL is immediate-mode.
    // For Vulkan, this would submit the command buffer + present.
    void* backendContext = nullptr;
    if (m_RenderGraph)
        backendContext = m_RenderGraph->GetBackendContext();

    if (backendContext)
    {
        auto* context = static_cast<GraphicsContext*>(backendContext);
        context->SwapBuffers();
    }

    // Release frame context
    m_FrameContext.reset();
}

void Renderer::OnWindowResize(uint32_t width, uint32_t height)
{
    VKT_PROFILE_FUNCTION();

    m_RendererAPI->SetViewport(0, 0, width, height);

    // Resize all registered framebuffers in the RenderGraph
    // (e.g. editor Viewport, off-screen render targets).
    // Passes automatically receive the updated FB via GetFramebuffer(key).
    if (m_RenderGraph)
        m_RenderGraph->ResizeAllFramebuffers(width, height);
}

} // namespace Vulkitten
