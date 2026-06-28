#pragma once

#include "Vulkitten/Core/Core.h"

#include <cstdint>

namespace Vulkitten {

// ============================================================
// SurfaceDesc - describes a platform drawable surface.
//
// Minimal for now; will be extended with format, sample count,
// and usage flags for Vulkan swapchain configuration.
// ============================================================
struct SurfaceDesc
{
    uint32_t Width = 0;
    uint32_t Height = 0;
};

// ============================================================
// ISurface - platform drawable surface abstraction.
//
// Represents an OS window's drawable region. For GLFW/Windows
// this wraps the native HWND; for Vulkan this is used to create
// VkSurfaceKHR via platform-specific extensions.
// ============================================================
class ISurface
{
public:
    virtual ~ISurface() = default;

    virtual SurfaceDesc GetDesc() const = 0;

    // Platform-native handle (HWND, GLFWwindow*, xcb_window_t, etc.)
    // Used by backends to create API-specific surfaces.
    virtual void* GetNativeHandle() const = 0;
};

} // namespace Vulkitten
