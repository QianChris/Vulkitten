#pragma once
#include <stdio.h>
#include "Vulkitten/Perf/Instrumentor.h"

#ifdef VULKITTEN_PLATFORM_WINDOWS
    extern Vulkitten::Application* Vulkitten::CreateApplication();

    int main()
    {
        VKT_PROFILE_BEGIN_SESSION("Vulkitten Startup", "vulkitten_profile_startup.json");
		Vulkitten::Log::Initialize();
        VKT_CORE_INFO("Vulkitten Engine Starting...");
        auto app = Vulkitten::CreateApplication();
        VKT_PROFILE_END_SESSION();

        VKT_PROFILE_BEGIN_SESSION("Run Application", "vulkitten_profile_runtime.json");
        app->Run();
        VKT_PROFILE_END_SESSION();

        VKT_PROFILE_BEGIN_SESSION("Vulkitten Shutdown", "vulkitten_profile_shutdown.json");
        VKT_CORE_INFO("Vulkitten Engine Stopping...");
        delete app;
        VKT_PROFILE_END_SESSION();

        return 0;
    }
#else
    #error Vulkitten only supports Windows!
#endif