#pragma once

#ifdef VULKITTEN_PLATFORM_WINDOWS
    #ifdef VULKITTEN_BUILD_DLL
        #define VulkittenAPI __declspec(dllexport)
    #else
        #define VulkittenAPI __declspec(dllimport)
    #endif
#else
    #error Vulkitten only supports Windows!
#endif