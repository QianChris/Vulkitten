#pragma once
#include <stdio.h>

#ifdef VULKITTEN_PLATFORM_WINDOWS
    extern Vulkitten::Application* Vulkitten::CreateApplication();

    int main()
    {
		Vulkitten::Log::Initialize();
        VKT_CORE_INFO("Vulkitten Engine Starting...");
        VKT_WARN("warn");

        auto app = Vulkitten::CreateApplication();
        app->Run();

        VKT_CORE_INFO("Vulkitten Engine Stopping...");
        delete app;
    }
#else
    #error Vulkitten only supports Windows!
#endif