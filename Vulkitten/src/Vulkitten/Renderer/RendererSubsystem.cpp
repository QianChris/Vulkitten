#include "vktpch.h"
#include "RendererSubsystem.h"

#include "Vulkitten/Perf/Instrumentor.h"

namespace Vulkitten {

RendererSubsystem* RendererSubsystem::s_Instance = nullptr;

RendererSubsystem::RendererSubsystem(const RendererConfig& config)
    : m_Renderer(config)
{
    s_Instance = this;
}

void RendererSubsystem::Init()
{
    VKT_PROFILE_FUNCTION();
    m_Renderer.Init();
}

void RendererSubsystem::Shutdown()
{
    VKT_PROFILE_FUNCTION();
    m_Renderer.Shutdown();
}

void RendererSubsystem::Execute()
{
    m_Renderer.Execute();
}

void RendererSubsystem::OnWindowResize(uint32_t width, uint32_t height)
{
    m_Renderer.OnWindowResize(width, height);
}

} // namespace Vulkitten
