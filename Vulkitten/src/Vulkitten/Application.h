#pragma once
#include "Vulkitten/Core.h"
#include "Vulkitten/Window.h"
#include "Vulkitten/LayerStack.h"
#include "Vulkitten/Events/Event.h"

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
    private:
        bool OnWindowClose(WindowCloseEvent& e);

		std::unique_ptr<Window> m_Window;
        bool m_Running = true;
        LayerStack m_LayerStack;
    };

    // to be defined in client
    Application* CreateApplication();
}