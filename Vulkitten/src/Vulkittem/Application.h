#pragma once
#include "Core.h"

namespace Vulkitten
{
    class VulkitenAPI Application
    {
    public:
        Application();
        virtual ~Application();

        void Run();
    };

    // to be defined in client
    Application* CreateApplication();
}