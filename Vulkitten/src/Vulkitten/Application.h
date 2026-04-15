#pragma once
#include "Vulkitten/Core.h"
#include "Vulkitten/Events/Event.h"

namespace Vulkitten
{
    class VKT_API Application
    {
    public:
        Application();
        virtual ~Application();

        void Run();
    };

    // to be defined in client
    Application* CreateApplication();
}