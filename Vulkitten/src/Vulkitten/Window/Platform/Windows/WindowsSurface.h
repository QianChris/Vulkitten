#pragma once

#include "Vulkitten/Core/ISurface.h"

struct GLFWwindow;

namespace Vulkitten {

// ============================================================
// WindowsSurface — GLFW-based ISurface implementation.
//
// Wraps a GLFWwindow as the platform drawable surface.
// GetNativeHandle() returns the GLFWwindow* for OpenGL context
// operations and Vulkan surface creation.
// ============================================================
class WindowsSurface : public ISurface
{
public:
    explicit WindowsSurface(GLFWwindow* window);
    virtual ~WindowsSurface();

    virtual SurfaceDesc GetDesc() const override;
    virtual void* GetNativeHandle() const override;

private:
    GLFWwindow* m_Window = nullptr;
};

} // namespace Vulkitten
