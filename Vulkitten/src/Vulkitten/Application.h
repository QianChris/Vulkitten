#pragma once
#include "Vulkitten/Core.h"
#include "Vulkitten/Window.h"

#include "Vulkitten/Events/Event.h"
#include "Vulkitten/Events/ApplicationEvent.h"
#include "Vulkitten/Events/KeyEvent.h"
#include "Vulkitten/Events/MouseEvent.h"

namespace Vulkitten
{
    class VKT_API Application
    {
    public:
        Application();
        virtual ~Application();

        void Run();

		void OnEvent(Event& e);
    private:
        bool OnWindowClose(WindowCloseEvent& e);

		std::unique_ptr<Window> m_Window;
        bool m_Running = true;
    };

    // to be defined in client
    Application* CreateApplication();
}