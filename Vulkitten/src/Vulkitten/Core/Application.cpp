#include "vktpch.h"
#include "Vulkitten/Core/Application.h"

#include "Vulkitten/Core/Layer.h"
#include "Vulkitten/Core/Engine.h"
#include "Vulkitten/Core/Input.h"
#include "Vulkitten/Core/IWindow.h"

#include "Vulkitten/Scene/SceneContext.h"

#include "Vulkitten/Renderer/IRenderer.h"
#include "Vulkitten/Renderer/IGpuResourceManager.h"
#include "Vulkitten/Renderer/Renderer.h"
#include "Vulkitten/Renderer/Backend/Vulkan/VkRenderer.h"
#include "Vulkitten/Renderer/ShaderManager.h"

#include <glm/glm.hpp>

#include "Vulkitten/Core/FileSystem.h"
#include "Vulkitten/Perf/Instrumentor.h"

namespace Vulkitten
{
    Application* Application::s_Instance = nullptr;
    RendererBackend Application::s_Backend = RendererBackend::OpenGL;

    Application::Application()
    {
        VKT_PROFILE_FUNCTION();

        Engine::Get().Init();
        Engine::Get().GetFileSystem().RegisterPath("../../Sandbox", "sandbox");

        VKT_ASSERT(!s_Instance, "Application already exists!");
        s_Instance = this;

        m_Window.reset(Window::Create());
        m_Window->SetEventCallback(VKT_BIND_EVENT_FN(Application::OnEvent));

        // Create shader manager (shared, backend-agnostic)
        m_ShaderMgr = CreateScope<ShaderManager>(Engine::Get().GetFileSystem());

        // Build RendererConfig
        RendererConfig config;
        config.FileSys  = &Engine::Get().GetFileSystem();
        config.Window   = dynamic_cast<IWindow*>(m_Window.get());
        config.ShaderMgr = m_ShaderMgr.get();

        // Create backend-specific IRenderer implementation
        if (s_Backend == RendererBackend::Vulkan)
        {
            m_Renderer = CreateScope<VkRenderer>(config);
        }
        else
        {
            m_Renderer = CreateScope<Renderer>(config);
        }

        m_Renderer->Init();

        m_ImGuiLayer = new ImGuiLayer();
        PushOverlay(m_ImGuiLayer);
    }

    Application::~Application()
    {
        VKT_PROFILE_FUNCTION();

        if (m_Renderer)
            m_Renderer->Shutdown();
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

            if (!m_Minimized && m_Renderer)
            {
                // ---- IRenderer Lifecycle: BeginFrame ----
                m_Renderer->BeginFrame();

                // SceneContext for dependency injection
                auto* graph = m_Renderer->GetRenderGraph();
                if (graph)
                {
                    SceneContext sceneCtx(*m_Renderer, *graph);
                    for (Layer *layer : m_LayerStack) {
                        VKT_PROFILE_SCOPE("Layer update");
                        layer->OnUpdate(timestep, sceneCtx);
                    }
                }
            }
            {
                VKT_PROFILE_SCOPE("Imgui update");
                m_ImGuiLayer->Begin();
                for (Layer* layer : m_LayerStack)
                    layer->OnImguiRender();
                m_ImGuiLayer->End();
            }

            // Execute + EndFrame via IRenderer
            if (m_Renderer)
            {
                m_Renderer->Execute();
                m_Renderer->EndFrame();
            }

            // GpuResourceManager GC
            if (m_Renderer)
            {
                m_Renderer->GetResourceManager().TickFrame();
                m_Renderer->GetResourceManager().Gc(3);
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
        if (m_Renderer)
            m_Renderer->OnWindowResize(e.GetWidth(), e.GetHeight());
        return false;
    }
}