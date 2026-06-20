#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Core/ISurface.h"

namespace Vulkitten {

// ============================================================
// IWindow — platform window interface for backend consumption.
//
// Separated from the existing Window class to provide a focused
// surface-oriented interface. The backend (Vulkan / OpenGL)
// interacts with the window ONLY through IWindow to query surface
// properties and obtain the drawable surface.
//
// Concrete implementations (e.g. WindowsWindow) implement both
// Window (for the application layer) and IWindow (for the backend).
// ============================================================
class IWindow
{
public:
    virtual ~IWindow() = default;

    // Query the current surface description (dimensions, etc.)
    virtual SurfaceDesc GetSurfaceDesc() const = 0;

    // Retrieve the platform drawable surface.
    // Ownership stays with the window; the returned pointer
    // is valid for the lifetime of the window.
    virtual ISurface* GetSurface() = 0;
};

} // namespace Vulkitten
