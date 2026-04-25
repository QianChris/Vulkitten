#pragma once

#include <memory>

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

namespace Vulkitten {
    template<typename T>
    using Scope = std::unique_ptr<T>;

    template<typename T>
    using Ref = std::shared_ptr<T>;
}

#include "Vulkitten/Assert.h"