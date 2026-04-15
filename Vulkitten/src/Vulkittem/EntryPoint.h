#pragma once

#ifdef VULKITTEN_PLATFORM_WINDOWS
    extern Vulkitten::Application* Vulkitten::CreateApplication();

    int main()
    {
        auto app = Vulkitten::CreateApplication();
        app->Run();
        delete app;
    }
#else
    #error Vulkitten only supports Windows!
#endif