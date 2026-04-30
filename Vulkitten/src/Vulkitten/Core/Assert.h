#pragma once

#include "Vulkitten/Core/Log.h"

#ifdef VULKITTEN_ENABLE_ASSERTS
    #define VKT_ASSERT(x, ...) { if(!(x)) { VKT_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
    #define VKT_CORE_ASSERT(x, ...) { if(!(x)) { VKT_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
    #define VKT_ASSERT(x, ...)
    #define VKT_CORE_ASSERT(x, ...)
#endif