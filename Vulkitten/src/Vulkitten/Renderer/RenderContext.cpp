#include "vktpch.h"
#include "RenderContext.h"

#include "Vulkitten/Renderer/Device.h"
#include "Vulkitten/Core/Application.h"

#include "Vulkitten/Perf/Instrumentor.h"

namespace Vulkitten {

RenderContext* RenderContext::s_Instance = nullptr;

RenderContext::RenderContext(Device* device, GpuResourceManager& resources, ShaderManager& shaders)
    : m_Renderer(device, resources, shaders)
{
    s_Instance = this;
}

void RenderContext::Init()
{
    VKT_PROFILE_FUNCTION();

    m_Renderer.Init();

    // Set backend context for EndPass (SwapBuffers)
    if (auto* graph = m_Renderer.GetRenderGraph())
    {
        graph->SetBackendContext(Application::Get().GetWindow().GetGraphicsContext());
    }
}

void RenderContext::Shutdown()
{
    VKT_PROFILE_FUNCTION();

    m_Renderer.Shutdown();
}

void RenderContext::Execute()
{
    m_Renderer.Execute();
}

void RenderContext::OnWindowResize(uint32_t width, uint32_t height)
{
    m_Renderer.OnWindowResize(width, height);
}

} // namespace Vulkitten
