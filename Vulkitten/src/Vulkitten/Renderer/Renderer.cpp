#include "vktpch.h"
#include "Renderer.h"
#include "Vulkitten/Renderer/Device.h"

namespace Vulkitten {

Renderer::Renderer(const RendererConfig& config)
    : m_Config(config)
{
}

Renderer::~Renderer()
{
}

IDevice& Renderer::GetDevice()
{
    return *m_Device;
}

IGpuResourceManager& Renderer::GetResourceManager()
{
    return *m_Resources;
}

void Renderer::OnWindowResize(uint32_t width, uint32_t height)
{
    if (m_RenderGraph)
        m_RenderGraph->ResizeAllFramebuffers(width, height);
}

} // namespace Vulkitten
