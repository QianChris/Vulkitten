#pragma once
#include "Vulkitten/Core.h"
#include "Vulkitten/Window.h"
#include "Vulkitten/LayerStack.h"
#include "Vulkitten/Events/Event.h"

#include "Vulkitten/ImGui/ImGuiLayer.h"
#include "Vulkitten/Renderer/Shader.h"

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

		std::unique_ptr<Window> m_Window;
        ImGuiLayer* m_ImGuiLayer;
        bool m_Running = true;
        LayerStack m_LayerStack;

		unsigned int m_VertexArray, m_VertexBuffer, m_IndexBuffer;
        std::unique_ptr<Shader> m_Shader;
    private:
        static Application* s_Instance;
    };

    // to be defined in client
    Application* CreateApplication();
}