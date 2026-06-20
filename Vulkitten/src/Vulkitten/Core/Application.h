#pragma once
#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Core/Window.h"
#include "Vulkitten/Core/LayerStack.h"
#include "Vulkitten/Events/Event.h"
#include "Vulkitten/Core/Timestep.h"

#include "Vulkitten/ImGui/ImGuiLayer.h"

#include <chrono>

namespace Vulkitten
{
    class RendererSubsystem;
    class GraphicContext;
    class OpenGLDevice;
    class GpuResourceManager;
    class ShaderManager;
    class VulkanInstance;
    class VkRenderer;
    class IRenderer;

    // Backend selection
    enum class RendererBackend { OpenGL, Vulkan };

    class VKT_API Application
    {
    public:
        Application();
        virtual ~Application();

        void Run();
        void SetClose();

		void OnEvent(Event& e);

        void PushLayer(Layer* layer);
        void PushOverlay(Layer* overlay);

        static Application& Get() { return *s_Instance; }
        inline Window& GetWindow() { return *m_Window; }
        inline float GetFPS() const { return m_FPS; }
        inline float GetFrameTime() const { return m_FrameTime; }

        // Set backend before constructing app (called by client)
        static void SetBackend(RendererBackend backend) { s_Backend = backend; }
        static RendererBackend GetBackend() { return s_Backend; }

    private:
        bool OnWindowClose(WindowCloseEvent& e);
        bool OnWindowResize(WindowResizeEvent& e);

		Scope<RendererSubsystem> m_RendererSubsystem;
        Scope<OpenGLDevice> m_Device;
        Scope<GpuResourceManager> m_Resources;
        Scope<ShaderManager> m_ShaderMgr;

		// Vulkan backend (optional)
		Scope<VulkanInstance> m_VulkanInstance;
		Scope<VkRenderer> m_VkRenderer;

		Scope<GraphicContext> m_GraphicContext;
		Scope<Window> m_Window;
        ImGuiLayer* m_ImGuiLayer;
        LayerStack m_LayerStack;

        bool m_Running = true;
        bool m_Minimized = false;

        std::chrono::high_resolution_clock::time_point m_FrameStartTime;
        std::chrono::high_resolution_clock::time_point m_LastFrameTime;
        float m_FPS = 0.0f;
        float m_FrameTime = 0.0f;
        float m_FrameCount = 0.0f;
        float m_FrameTimeAccumulator = 0.0f;

    private:
        static Application* s_Instance;
        static RendererBackend s_Backend;
    };

    // to be defined in client
    Application* CreateApplication();
}