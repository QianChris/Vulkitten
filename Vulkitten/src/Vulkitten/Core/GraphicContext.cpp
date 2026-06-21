#include "vktpch.h"
#include "GraphicContext.h"
#include "Vulkitten/Renderer/IRenderer.h"

namespace Vulkitten {

GraphicContext* GraphicContext::s_Instance = nullptr;

GraphicContext::GraphicContext(Window& window, RendererSubsystem& /*renderSubsystem*/)
    : m_Window(window)
{
    s_Instance = this;
}

} // namespace Vulkitten
