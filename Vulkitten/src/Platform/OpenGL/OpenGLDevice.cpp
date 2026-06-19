#include "vktpch.h"
#include "OpenGLDevice.h"

#include "Vulkitten/Perf/Instrumentor.h"

namespace Vulkitten {

void OpenGLDevice::Init()
{
    VKT_PROFILE_RENDER_FUNCTION();

    // Placeholder: the actual OpenGL init is currently handled by
    // OpenGLContext::Init() (GLAD loading) + RendererAPI::Init() (glEnable).
    // When RenderContext wires Device in a later task, this will become the
    // single entry point for GPU device initialization.
}

void OpenGLDevice::Shutdown()
{
    VKT_PROFILE_RENDER_FUNCTION();

    // Placeholder: GPU resource cleanup. Currently handled by
    // Renderer::Shutdown() → Renderer2D::Shutdown().
    // Will take over that role when GpuResourceManager is wired in.
}

} // namespace Vulkitten
