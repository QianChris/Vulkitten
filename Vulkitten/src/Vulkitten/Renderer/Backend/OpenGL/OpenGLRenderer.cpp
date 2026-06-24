#include "vktpch.h"
#include "OpenGLRenderer.h"
#include "Vulkitten/Renderer/Backend/OpenGL/OpenGLDevice.h"
#include "Vulkitten/Renderer/Backend/OpenGL/OpenGLGpuResourceManager.h"
#include "Vulkitten/Core/IWindow.h"
#include "Vulkitten/Core/ISurface.h"

#include "Vulkitten/Renderer/Passes/PreparePass.h"
#include "Vulkitten/Renderer/Passes/GpuParticlePass.h"
#include "Vulkitten/Renderer/Passes/SpriteRenderPass.h"
#include "Vulkitten/Perf/Instrumentor.h"

namespace Vulkitten {

static uint32_t s_GlobalFrameIndex = 0;

OpenGLRenderer::OpenGLRenderer(const RendererConfig& config)
    : Renderer(config)
{
}

OpenGLRenderer::~OpenGLRenderer()
{
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

    m_RenderGraph = new RenderGraph();

    // Register default render passes
    m_RenderGraph->AddPass(CreateRef<PreparePass>());
    m_RenderGraph->AddPass(CreateRef<GpuParticlePass>());
    m_RenderGraph->AddPass(CreateRef<SpriteRenderPass>());

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

    // [HACK: 过渡期 — Task 15: call Renderer::BeginFrame() which delegates to IDevice::beginFrame()]
    //Renderer::BeginFrame();
}

void OpenGLRenderer::Execute()
{
    if (m_RenderGraph)
        m_RenderGraph->Execute();
}

void OpenGLRenderer::EndFrame()
{
    VKT_PROFILE_FUNCTION();

    Renderer::EndFrame();  // Calls IDevice::endFrame(ctx) → glfwSwapBuffers
}

void OpenGLRenderer::OnWindowResize(uint32_t width, uint32_t height)
{
    VKT_PROFILE_FUNCTION();

    Renderer::OnWindowResize(width, height);  // Calls IDevice::onResize + RenderGraph::ResizeAllFramebuffers
}

} // namespace Vulkitten
