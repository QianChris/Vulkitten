#pragma once

#include "Vulkitten/Core/Window.h"
#include "Vulkitten/Core/IWindow.h"
#include "Platform/OpenGL/OpenGLContext.h"

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace Vulkitten
{
    class WindowsSurface;

    class WindowsWindow : public Window, public IWindow
    {
    public:
        WindowsWindow(const WindowProps& props);
        virtual ~WindowsWindow();

        void OnUpdate() override;

        unsigned int GetWidth() const override { return m_Data.Width; }
        unsigned int GetHeight() const override { return m_Data.Height; }

        inline virtual void* GetNativeWindow() const override { return m_Window; }
        inline virtual void* GetGraphicsContext() const override { return m_Context; }

        // Window attributes
        void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
        void SetVSync(bool enabled) override;
        bool IsVSync() const override;

        // ---- IWindow interface ----
        SurfaceDesc GetSurfaceDesc() const override;
        ISurface* GetSurface() override;

    private:
        virtual void Init(const WindowProps& props);
        virtual void Shutdown();

    private:
        GLFWwindow* m_Window;
        OpenGLContext* m_Context;
        Scope<WindowsSurface> m_Surface;

        struct WindowData
        {
            std::string Title;
            unsigned int Width, Height;
            bool VSync;

            EventCallbackFn EventCallback;
        };

        WindowData m_Data;
    };

}