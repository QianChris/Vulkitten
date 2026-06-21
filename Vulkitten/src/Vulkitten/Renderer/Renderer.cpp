#include "vktpch.h"
#include "Renderer.h"
#include "Vulkitten/Renderer/Backend/OpenGL/OpenGLRendererAPI.h"
#include "Vulkitten/Renderer/Backend/OpenGL/OpenGLDevice.h"
#include "Vulkitten/Renderer/GpuResourceManager.h"
#include "Vulkitten/Renderer/ShaderManager.h"

#include "Vulkitten/Renderer/Passes/PreparePass.h"
#include "Vulkitten/Renderer/Passes/GpuParticlePass.h"
#include "Vulkitten/Renderer/Passes/SpriteRenderPass.h"
#include "Vulkitten/Renderer/Passes/EndPass.h"

#include "Vulkitten/Renderer/GraphicsContext.h"
#include "Vulkitten/Core/Application.h"

#include "Vulkitten/Perf/Instrumentor.h"

namespace Vulkitten {

static uint32_t s_GlobalFrameIndex = 0;

Renderer::Renderer(const RendererConfig& config)
    : m_Config(config)
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

    // Register as global IRenderer instance FIRST — Pass constructors
    // and resource creation call IRenderer::Get() internally.
    s_Current = this;

    // Create OpenGL backend dependencies internally
    m_Device = CreateScope<OpenGLDevice>();
    m_Resources = CreateScope<GpuResourceManager>();

    m_RendererAPI = new OpenGLRendererAPI();
    m_RendererAPI->Init();

    m_RenderGraph = new RenderGraph();

    // Register default render passes
    m_RenderGraph->AddPass(CreateRef<PreparePass>());
    m_RenderGraph->AddPass(CreateRef<GpuParticlePass>());
    m_RenderGraph->AddPass(CreateRef<SpriteRenderPass>());
    m_RenderGraph->AddPass(CreateRef<EndPass>());

    // Preload engine shaders
    m_ShaderLibrary.Add("ParticleSimArg",
        Shader::CreateCompute("ParticleSimArg",
            "engine://computeshaders/ParticleSimArg.comp"));
    m_ShaderLibrary.Add("ParticleSim",
        Shader::CreateCompute("ParticleSim",
            "engine://computeshaders/ParticleSim.comp"));
    m_ShaderLibrary.Add("ParticleEmit",
        Shader::CreateCompute("ParticleEmit",
            "engine://computeshaders/ParticleEmit.comp"));
    m_ShaderLibrary.Add("ParticleRenderArg",
        Shader::CreateCompute("ParticleRenderArg",
            "engine://computeshaders/ParticleRenderArg.comp"));
    m_ShaderLibrary.Load("engine://shaders/Particle.shader");

    // Set backend context for EndPass (SwapBuffers)
    if (m_RenderGraph)
        m_RenderGraph->SetBackendContext(Application::Get().GetWindow().GetGraphicsContext());

    VKT_CORE_INFO("Renderer: OpenGL backend initialized");
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

    void* backendContext = nullptr;
    if (m_RenderGraph)
        backendContext = m_RenderGraph->GetBackendContext();

    if (backendContext)
    {
        auto* context = static_cast<GraphicsContext*>(backendContext);
        context->SwapBuffers();
    }

    m_FrameContext.reset();
}

void Renderer::OnWindowResize(uint32_t width, uint32_t height)
{
    VKT_PROFILE_FUNCTION();

    m_RendererAPI->SetViewport(0, 0, width, height);

    if (m_RenderGraph)
        m_RenderGraph->ResizeAllFramebuffers(width, height);
}

IDevice& Renderer::GetDevice()
{
    return *m_Device;
}

IGpuResourceManager& Renderer::GetResourceManager()
{
    return *m_Resources;
}

} // namespace Vulkitten
