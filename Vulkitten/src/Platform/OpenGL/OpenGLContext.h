#pragma once

#include "Vulkitten/Core.h"
#include "Vulkitten/Renderer/GraphicsContext.h"

struct GLFWwindow;

namespace Vulkitten {

    class OpenGLContext : public GraphicsContext
    {
    public:
        OpenGLContext(GLFWwindow* windowHandle);
        virtual ~OpenGLContext();

        virtual void Init() override;
        virtual void SwapBuffers() override;

    private:
        GLFWwindow* m_WindowHandle;
    };

}