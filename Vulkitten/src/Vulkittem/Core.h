#pragma once

#ifdef VULKITTEN_PLATFORM_WINDOWS
    #ifdef VULKITTEN_BUILD_DLL
        #define VulkitenAPI __declspec(dllexport)
    #else
        #define VulkitenAPI __declspec(dllimport)
    #endif
#else
    #error Vulkitten only supports Windows!
#endif