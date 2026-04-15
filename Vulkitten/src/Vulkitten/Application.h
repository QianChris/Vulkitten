#pragma once
#include "Vulkitten/Core.h"
#include "Vulkitten/Events/Event.h"
#include "Vulkitten/Window.h"

namespace Vulkitten
{
    class VKT_API Application
    {
    public:
        Application();
        virtual ~Application();

        void Run();
    private:
		std::unique_ptr<Window> m_Window;
        bool m_Running = true;
    };

    // to be defined in client
    Application* CreateApplication();
}