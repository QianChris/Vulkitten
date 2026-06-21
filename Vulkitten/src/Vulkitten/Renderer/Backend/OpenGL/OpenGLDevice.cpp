#include "vktpch.h"
#include "OpenGLDevice.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "Vulkitten/Perf/Instrumentor.h"

namespace Vulkitten {

OpenGLDevice::OpenGLDevice(void* nativeWindow)
    : m_NativeWindow(nativeWindow)
{
}

void OpenGLDevice::Init()
{
    VKT_PROFILE_RENDER_FUNCTION();
}

void OpenGLDevice::Shutdown()
{
    VKT_PROFILE_RENDER_FUNCTION();
}

void OpenGLDevice::Submit(FrameContext& /*frameContext*/)
{
    if (m_NativeWindow)
    {
        glfwSwapBuffers(static_cast<GLFWwindow*>(m_NativeWindow));
    }
}

} // namespace Vulkitten
