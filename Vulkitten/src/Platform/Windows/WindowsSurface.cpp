#include "vktpch.h"
#include "WindowsSurface.h"

#include <GLFW/glfw3.h>

namespace Vulkitten {

WindowsSurface::WindowsSurface(GLFWwindow* window)
    : m_Window(window)
{
}

WindowsSurface::~WindowsSurface()
{
}

SurfaceDesc WindowsSurface::GetDesc() const
{
    SurfaceDesc desc;
    if (m_Window)
    {
        int w, h;
        glfwGetWindowSize(m_Window, &w, &h);
        desc.Width = static_cast<uint32_t>(w);
        desc.Height = static_cast<uint32_t>(h);
    }
    return desc;
}

void* WindowsSurface::GetNativeHandle() const
{
    return static_cast<void*>(m_Window);
}

} // namespace Vulkitten
