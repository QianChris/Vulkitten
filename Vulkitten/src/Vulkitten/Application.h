#pragma once
#include "Vulkitten/Core.h"
#include "Vulkitten/Window.h"
#include "Vulkitten/LayerStack.h"
#include "Vulkitten/Events/Event.h"
#include "Vulkitten/Core/Timestep.h"

#include "Vulkitten/ImGui/ImGuiLayer.h"
#include "Vulkitten/Renderer/Shader.h"
#include "Vulkitten/Renderer/Buffer.h"
#include "Vulkitten/Renderer/VertexArray.h"
#include "Vulkitten/Renderer/OrthographicCamera.h"

#include <chrono>

namespace Vulkitten
{
    class VKT_API Application
    {
    public:
        Application();
        virtual ~Application();

        void Run();

		void OnEvent(Event& e);

        void PushLayer(Layer* layer);
        void PushOverlay(Layer* overlay);

        static Application& Get() { return *s_Instance; }
        inline Window& GetWindow() { return *m_Window; }
    private:
        bool OnWindowClose(WindowCloseEvent& e);

		Scope<Window> m_Window;
        ImGuiLayer* m_ImGuiLayer;
        bool m_Running = true;
        LayerStack m_LayerStack;

        std::chrono::high_resolution_clock::time_point m_LastFrameTime;

    private:
        static Application* s_Instance;
    };

    // to be defined in client
    Application* CreateApplication();
}