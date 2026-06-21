#include "vktpch.h"
#include "OpenGLRenderer.h"
#include "Vulkitten/Renderer/Backend/OpenGL/OpenGLRendererAPI.h"
#include "Vulkitten/Renderer/Backend/OpenGL/OpenGLDevice.h"
#include "Vulkitten/Renderer/Backend/OpenGL/OpenGLGpuResourceManager.h"
#include "Vulkitten/Core/IWindow.h"
#include "Vulkitten/Core/ISurface.h"

#include "Vulkitten/Renderer/Passes/PreparePass.h"
#include "Vulkitten/Renderer/Passes/GpuParticlePass.h"
#include "Vulkitten/Renderer/Passes/SpriteRenderPass.h"
#include "Vulkitten/Renderer/Passes/EndPass.h"

#include "Vulkitten/Perf/Instrumentor.h"

namespace Vulkitten {

static uint32_t s_GlobalFrameIndex = 0;

OpenGLRenderer::OpenGLRenderer(const RendererConfig& config)
    : Renderer(config)
{
}

OpenGLRenderer::~OpenGLRenderer()
{
    delete m_RendererAPI;
    m_RendererAPI = nullptr;
}

void OpenGLRenderer::Init()
{
    VKT_PROFILE_FUNCTION();

    // Register as global IRenderer instance FIRST — Pass constructors
    // and resource creation call IOpenGLRenderer::Get() internally.
    s_Current = this;

    // Create OpenGL backend dependencies internally
    void* nativeWindow = m_Config.Window
        ? m_Config.Window->GetSurface()->GetNativeHandle()
        : nullptr;
    m_Device = CreateScope<OpenGLDevice>(nativeWindow);
    m_Resources = CreateScope<OpenGLGpuResourceManager>(*m_Config.FileSys);

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
    m_ShaderLibrary.Load("engine://shaders/Particle");

    VKT_CORE_INFO("OpenGLRenderer: OpenGL backend initialized");
}

void OpenGLRenderer::Shutdown()
{
    VKT_PROFILE_FUNCTION();

    delete m_RenderGraph;
    m_RenderGraph = nullptr;
}

void OpenGLRenderer::BeginFrame()
{
    VKT_PROFILE_FUNCTION();

    m_FrameContext = CreateScope<FrameContext>();
    m_FrameContext->FrameIndex = s_GlobalFrameIndex++;
}

void OpenGLRenderer::Execute()
{
    if (m_RenderGraph)
        m_RenderGraph->Execute();
}

void OpenGLRenderer::EndFrame()
{
    VKT_PROFILE_FUNCTION();

    // Submit to device (SwapBuffers for OpenGL)
    if (m_Device && m_FrameContext)
        m_Device->Submit(*m_FrameContext);

    m_FrameContext.reset();
}

void OpenGLRenderer::OnWindowResize(uint32_t width, uint32_t height)
{
    VKT_PROFILE_FUNCTION();

    m_RendererAPI->SetViewport(0, 0, width, height);

    // Base class: resize all registered framebuffers
    Renderer::OnWindowResize(width, height);
}

} // namespace Vulkitten
