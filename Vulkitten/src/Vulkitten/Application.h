#pragma once
#include "Vulkitten/Core.h"
#include "Vulkitten/Window.h"
#include "Vulkitten/LayerStack.h"
#include "Vulkitten/Events/Event.h"

#include "Vulkitten/ImGui/ImGuiLayer.h"
#include "Vulkitten/Renderer/Shader.h"
#include "Vulkitten/Renderer/Buffer.h"
#include "Vulkitten/Renderer/VertexArray.h"

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

        std::shared_ptr<Shader> m_Shader;
        std::shared_ptr<VertexArray> m_VAO;

        std::shared_ptr<Shader> m_SquareShader;
        std::shared_ptr<VertexArray> m_SquareVAO;
    private:
        static Application* s_Instance;
    };

    // to be defined in client
    Application* CreateApplication();
}