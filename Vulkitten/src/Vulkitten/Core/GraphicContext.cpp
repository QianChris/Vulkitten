#include "vktpch.h"
#include "GraphicContext.h"
#include "Vulkitten/Renderer/RenderContext.h"

namespace Vulkitten {

GraphicContext* GraphicContext::s_Instance = nullptr;

GraphicContext::GraphicContext(Window& window, RenderContext& /*renderContext*/)
    : m_Window(window)
{
    s_Instance = this;
}

} // namespace Vulkitten
