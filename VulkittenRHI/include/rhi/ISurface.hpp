#pragma once

#include <cstdint>

namespace rhi {

// ============================================================
// SurfaceDesc - describes a platform drawable surface
// ============================================================

struct SurfaceDesc
{
    uint32_t Width = 0;
    uint32_t Height = 0;
};

// ============================================================
// ISurface - platform drawable surface abstraction
//
// Represents an OS window's drawable region.
//   OpenGL:  wraps the GLFWwindow for context creation
//   Vulkan:  used to create VkSurfaceKHR
// ============================================================

class ISurface
{
public:
    virtual ~ISurface() = default;

    virtual SurfaceDesc GetDesc() const = 0;

    // Platform-native handle (GLFWwindow*, HWND, xcb_window_t, etc.)
    virtual void* GetNativeHandle() const = 0;
};

} // namespace rhi
