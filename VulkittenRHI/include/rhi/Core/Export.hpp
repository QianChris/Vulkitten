#pragma once

// ============================================================
// RHI_API - DLL export/import macro for VulkittenRHI
//
// RHI_BUILD_DLL is defined only when building VulkittenRHI.dll.
// Consumers get __declspec(dllimport) automatically.
// ============================================================

#ifdef _WIN32
    #ifdef RHI_BUILD_DLL
        #define RHI_API __declspec(dllexport)
    #else
        #define RHI_API __declspec(dllimport)
    #endif
#else
    #define RHI_API
#endif
