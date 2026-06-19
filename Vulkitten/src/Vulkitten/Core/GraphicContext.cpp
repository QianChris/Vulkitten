#include "vktpch.h"
#include "GraphicContext.h"
#include "Vulkitten/Renderer/RenderContext.h"

namespace Vulkitten {

GraphicContext* GraphicContext::s_Instance = nullptr;

GraphicContext::GraphicContext(RenderContext& /*renderContext*/)
    : m_Window(Window::Create())
{
    s_Instance = this;
}

} // namespace Vulkitten
