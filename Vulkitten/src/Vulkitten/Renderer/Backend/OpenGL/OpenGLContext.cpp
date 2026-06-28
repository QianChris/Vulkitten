#include "vktpch.h"
#include "OpenGLContext.h"

#include "Vulkitten/Perf/Instrumentor.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Vulkitten {

    OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
        : m_WindowHandle(windowHandle)
    {
        VKT_PROFILE_RENDER_FUNCTION();

        VKT_CORE_ASSERT(windowHandle, "Window handle is null!");
    }

    OpenGLContext::~OpenGLContext()
    {
    }

    void OpenGLContext::Init()
    {
        VKT_PROFILE_RENDER_FUNCTION();

        glfwMakeContextCurrent(m_WindowHandle);
        int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
        VKT_CORE_ASSERT(status, "Failed to initialize Glad!");

        VKT_CORE_INFO("OpenGL Vendor: {0}", (char*)glGetString(GL_VENDOR));
		VKT_CORE_INFO("OpenGL Renderer: {0}", (char*)glGetString(GL_RENDERER));
        VKT_CORE_INFO("OpenGL Version: {0}", (char*)glGetString(GL_VERSION));
    }   

    void OpenGLContext::SwapBuffers()
    {
        VKT_PROFILE_RENDER_FUNCTION();

        glfwSwapBuffers(m_WindowHandle);
    }
}