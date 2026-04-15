#pragma once
#include "Core.h"

namespace Vulkitten
{
    class VulkittenAPI Application
    {
    public:
        Application();
        virtual ~Application();

        void Run();
    };

    // to be defined in client
    Application* CreateApplication();
}