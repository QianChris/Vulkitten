#include "Vulkitten/Application.h"

#include "Vulkitten/Log.h"
#include "Vulkitten/Events/ApplicationEvent.h"
#include "Vulkitten/Events/KeyEvent.h"
#include "Vulkitten/Events/MouseEvent.h"

namespace Vulkitten
{
    Application::Application()
    {
    }

    Application::~Application()
    {
    }

    void Application::Run()
    {
        WindowResizeEvent e(1280, 720);
        VKT_TRACE("{}", e.ToString());
		KeyPressedEvent e2(32, 1);
		VKT_TRACE("{}", e2.ToString());
		MouseButtonPressedEvent e3(1);
		VKT_TRACE("{}", e3.ToString());
        if (e.IsInCategory(EventCategoryApplication))
            VKT_TRACE("Event belongs to Application category!");
        if (e.IsInCategory(EventCategoryInput))
            VKT_TRACE("Event belongs to Input category!");

        while (true);
    }
}