#pragma once

#ifdef VULKITTEN_PLATFORM_WINDOWS
    #ifdef VULKITTEN_BUILD_DLL
        #define VKT_API __declspec(dllexport)
    #else
        #define VKT_API __declspec(dllimport)
    #endif
#else
    #error Vulkitten only supports Windows!
#endif

#define IMGUI_API VKT_API

#define BIT(x) (1 << x)

#include "Vulkitten/Assert.h"