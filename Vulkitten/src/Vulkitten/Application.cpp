#include "vktpch.h"
#include "Vulkitten/Application.h"

#include "Vulkitten/Layer.h"
#include "Vulkitten/Input.h"

#include "Vulkitten/Renderer/Renderer.h"
#include "Vulkitten/Renderer/RenderCommand.h"

#include <glm/glm.hpp>

namespace Vulkitten
{
    Application* Application::s_Instance = nullptr;

    Application::Application()
    {
        VKT_ASSERT(!s_Instance, "Application already exists!");
        s_Instance = this;

        m_Window = std::unique_ptr<Window>(Window::Create());
        m_Window->SetEventCallback(VKT_BIND_EVENT_FN(Application::OnEvent));

        m_ImGuiLayer = new ImGuiLayer();
        PushOverlay(m_ImGuiLayer);
    }

    Application::~Application()
    {
    }

    void Application::Run()
    {
        m_LastFrameTime = std::chrono::high_resolution_clock::now();

        while (m_Running)
        {
            auto time = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - m_LastFrameTime);
            Timestep timestep(time.count());
            m_LastFrameTime = std::chrono::high_resolution_clock::now();

            for (Layer *layer : m_LayerStack)
                layer->OnUpdate(timestep);

            m_ImGuiLayer->Begin();
            for (Layer *layer : m_LayerStack)
                layer->OnImguiRender();
            m_ImGuiLayer->End();

            m_Window->OnUpdate();

            //auto [mouseX, mouseY] = Input::GetMousePosition();
            //VKT_CORE_TRACE("{0}, {1}", mouseX, mouseY);
        }
    }

    void Application::OnEvent(Event &e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>(VKT_BIND_EVENT_FN(Application::OnWindowClose));

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
        m_LayerStack.PushLayer(layer);
        layer->OnAttach();
    }

    void Application::PushOverlay(Layer *overlay)
    {
        m_LayerStack.PushOverlay(overlay);
        overlay->OnAttach();
    }

    bool Application::OnWindowClose(WindowCloseEvent &e)
    {
        m_Running = false;
        return false;
    }
}