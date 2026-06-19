#include "vktpch.h"
#include "Vulkitten/Core/Application.h"

#include "Vulkitten/Core/Layer.h"
#include "Vulkitten/Core/Engine.h"
#include "Vulkitten/Core/Input.h"
#include "Vulkitten/Core/GraphicContext.h"

#include "Vulkitten/Renderer/RenderContext.h"
#include "Vulkitten/Renderer/Device.h"
#include "Platform/OpenGL/OpenGLDevice.h"
#include "Vulkitten/Renderer/GpuResourceManager.h"
#include "Vulkitten/Renderer/ShaderManager.h"
#include "Vulkitten/Renderer/RenderCommand.h"

#include <glm/glm.hpp>

#include "Vulkitten/Core/FileSystem.h"
#include "Vulkitten/Perf/Instrumentor.h"

namespace Vulkitten
{
    Application* Application::s_Instance = nullptr;

    Application::Application()
    {
        VKT_PROFILE_FUNCTION();

        // Init engine subsystems first
        Engine::Get().Init();
        Engine::Get().GetFileSystem().RegisterPath("../../Sandbox", "sandbox");

        VKT_ASSERT(!s_Instance, "Application already exists!");
        s_Instance = this;

         //Create window first (RenderContext::Init needs it for backend context)
        m_Window.reset(Window::Create());
        m_Window->SetEventCallback(VKT_BIND_EVENT_FN(Application::OnEvent));

        // Create renderer infrastructure
        m_Device      = CreateScope<OpenGLDevice>();
        m_Resources   = CreateScope<GpuResourceManager>();
        m_ShaderMgr   = CreateScope<ShaderManager>(Engine::Get().GetFileSystem());

        m_RenderContext = CreateScope<RenderContext>(
            m_Device.get(), *m_Resources, *m_ShaderMgr);
        m_RenderContext->Init();

        // GraphicContext wraps existing window + RenderContext (no duplicate window)
        m_GraphicContext = CreateScope<GraphicContext>(*m_Window, *m_RenderContext);

        m_ImGuiLayer = new ImGuiLayer();
        PushOverlay(m_ImGuiLayer);
    }

    Application::~Application()
    {
        VKT_PROFILE_FUNCTION();

        if (m_RenderContext)
            m_RenderContext->Shutdown();
    }

    void Application::Run()
    {
        VKT_PROFILE_FUNCTION();

        m_LastFrameTime = std::chrono::high_resolution_clock::now();

        while (m_Running)
        {
            auto startTime = std::chrono::high_resolution_clock::now();

            auto time = std::chrono::duration<float>(startTime - m_LastFrameTime);
            Timestep timestep(time.count());
            m_LastFrameTime = std::chrono::high_resolution_clock::now();

            m_FrameCount += 1.0f;
            m_FrameTimeAccumulator += timestep.GetSeconds();
            if (m_FrameTimeAccumulator >= 1.0f) {
                m_FPS = m_FrameCount / m_FrameTimeAccumulator;
                m_FrameCount = 0.0f;
                m_FrameTimeAccumulator = 0.0f;
            }

            if (!m_Minimized) {
                for (Layer *layer : m_LayerStack) {
                    VKT_PROFILE_SCOPE("Layer update");
                    layer->OnUpdate(timestep);
                }
            }
            {
                VKT_PROFILE_SCOPE("Imgui update");
                m_ImGuiLayer->Begin();
                for (Layer* layer : m_LayerStack)
                    layer->OnImguiRender();
                m_ImGuiLayer->End();
            }

            // Execute RenderGraph (all passes, including present via EndPass)
            {
                VKT_PROFILE_SCOPE("RenderGraph execute");
                m_RenderContext->Execute();
            }

            auto frameEndTime = std::chrono::high_resolution_clock::now();
            m_FrameTime = std::chrono::duration<float>(frameEndTime - startTime).count();

            {
                VKT_PROFILE_SCOPE("Window update");
                m_Window->OnUpdate();
            }
        }
    }

    void Application::SetClose()
    {
        m_Running = false;
    }

    void Application::OnEvent(Event &e)
    {
        VKT_PROFILE_FUNCTION();

        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>(VKT_BIND_EVENT_FN(Application::OnWindowClose));
        dispatcher.Dispatch<WindowResizeEvent>(VKT_BIND_EVENT_FN(Application::OnWindowResize));
        for (auto it = m_LayerStack.end(); it != m_LayerStack.begin();)
        {
            (*--it)->OnEvent(e);
            if (e.Handled)
                break;
        }

        //VKT_CORE_TRACE("{}", e.ToString());
    }

    void Application::PushLayer(Layer *layer)
    {
        VKT_PROFILE_FUNCTION();

        m_LayerStack.PushLayer(layer);
        layer->OnAttach();
    }

    void Application::PushOverlay(Layer *overlay)
    {
        VKT_PROFILE_FUNCTION();

        m_LayerStack.PushOverlay(overlay);
        overlay->OnAttach();
    }

    bool Application::OnWindowClose(WindowCloseEvent &e)
    {
        m_Running = false;
        return false;
    }

    bool Application::OnWindowResize(WindowResizeEvent &e)
    {
        if (e.GetWidth() == 0 || e.GetHeight() == 0)
        {
            m_Minimized = true;
            return false;
        }

        m_Minimized = false;
        if (m_RenderContext)
            m_RenderContext->OnWindowResize(e.GetWidth(), e.GetHeight());
        return false;
    }
}