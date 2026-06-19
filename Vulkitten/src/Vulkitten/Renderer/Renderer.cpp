#include "vktpch.h"
#include "Renderer.h"
#include "RenderCommand.h"
#include "Renderer2D.h"

#include "Vulkitten/Renderer/Passes/PreparePass.h"
#include "Vulkitten/Renderer/Passes/GpuParticlePass.h"
#include "Vulkitten/Renderer/Passes/SpriteRenderPass.h"
#include "Vulkitten/Renderer/Passes/EndPass.h"

#include "Vulkitten/Perf/Instrumentor.h"

namespace Vulkitten {

Renderer::Renderer(Device* device, GpuResourceManager& resources, ShaderManager& shaders)
    : m_Device(device)
    , m_Resources(resources)
    , m_Shaders(shaders)
{
}

void Renderer::Init()
{
    VKT_PROFILE_FUNCTION();

    Legacy::RenderCommand::Init();
    Renderer2D::Init();

    m_RenderGraph = new RenderGraph();

    // Register default render passes (order matters)
    m_RenderGraph->AddPass(PreparePass{});       // Clear
    m_RenderGraph->AddPass(GpuParticlePass{});   // GPU particle update + render
    m_RenderGraph->AddPass(SpriteRenderPass{});  // 2D quad batch
    m_RenderGraph->AddPass(EndPass{});           // SwapBuffers
}

void Renderer::Shutdown()
{
    VKT_PROFILE_FUNCTION();

    delete m_RenderGraph;
    m_RenderGraph = nullptr;

    Renderer2D::Shutdown();
}

void Renderer::Execute()
{
    if (m_RenderGraph)
        m_RenderGraph->Execute();
}

void Renderer::SetViewProjection(const glm::mat4& vp)
{
    if (m_RenderGraph)
        m_RenderGraph->SetViewProjection(vp);
}

void Renderer::OnWindowResize(uint32_t width, uint32_t height)
{
    VKT_PROFILE_FUNCTION();

    Legacy::RenderCommand::SetViewport(0, 0, width, height);
}

} // namespace Vulkitten
