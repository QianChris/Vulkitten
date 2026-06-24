// ============================================================
// VulkittenRHI Sample — Clear-to-Red
//
// Creates a GLFW window, clears it red each frame.
// Switch between OpenGL and Vulkan by changing ONE line:
//   config.Backend = BackendType::OpenGL  ← or BackendType::Vulkan
//
// Build:
//   cmake -S . -B build -G "Visual Studio 17 2022" -A x64
//   cmake --build build --config Debug
//
// Run:
//   ./bin/Debug-x64/Sample/Sample.exe
// ============================================================

#include <rhi/Renderer.hpp>
#include <rhi/IRenderDevice.hpp>
#include <rhi/ICommandBuffer.hpp>
#include <rhi/RenderCommandList.hpp>
#include <rhi/ISurface.hpp>
#include <rhi/Core/Types.hpp>
#include <rhi/Core/Format.hpp>

#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstdlib>
#include <stdexcept>

// ============================================================
// GLFWSurface — adapts GLFWwindow to rhi::ISurface
// ============================================================
class GLFWSurface : public rhi::ISurface
{
public:
    explicit GLFWSurface(GLFWwindow* window)
        : m_Window(window)
    {
        glfwGetFramebufferSize(window, &m_Width, &m_Height);
    }

    rhi::SurfaceDesc GetDesc() const override
    {
        rhi::SurfaceDesc desc;
        desc.Width  = static_cast<uint32_t>(m_Width);
        desc.Height = static_cast<uint32_t>(m_Height);
        return desc;
    }

    void* GetNativeHandle() const override
    {
        return static_cast<void*>(m_Window);
    }

    void UpdateSize()
    {
        glfwGetFramebufferSize(m_Window, &m_Width, &m_Height);
    }

private:
    GLFWwindow* m_Window = nullptr;
    int m_Width = 800;
    int m_Height = 600;
};

// ============================================================
// Helper: error callback
// ============================================================
static void GlfwErrorCallback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// ============================================================
// main
// ============================================================
int main()
{
    // ---- Select Backend (CHANGE THIS ONE LINE) ----
    //   BackendType::OpenGL  ← OpenGL
    //   BackendType::Vulkan  ← Vulkan
    //const rhi::BackendType BACKEND = rhi::BackendType::OpenGL;
    const rhi::BackendType BACKEND = rhi::BackendType::Vulkan;

    // ---- Initialize GLFW ----
    glfwSetErrorCallback(GlfwErrorCallback);
    if (!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return 1;
    }

    // GLFW window hints adapted to backend (does NOT affect RHI internals)
    if (BACKEND == rhi::BackendType::OpenGL)
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    }
    else
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }

    GLFWwindow* window = glfwCreateWindow(800, 600, "VulkittenRHI - Clear Red", nullptr, nullptr);
    if (!window)
    {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return 1;
    }

    // Wrap in ISurface
    GLFWSurface surface(window);

    // ---- Create Renderer ----
    rhi::RendererConfig config;
    config.Backend = BACKEND;
    config.Surface = &surface;

    std::unique_ptr<rhi::Renderer> renderer;
    try
    {
        renderer = rhi::Renderer::Create(config);
    }
    catch (const std::exception& e)
    {
        fprintf(stderr, "Failed to create renderer: %s\n", e.what());
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    printf("Renderer created successfully! Backend: %s\n",
           renderer->GetBackendType() == rhi::BackendType::OpenGL ? "OpenGL" : "Vulkan");

    // ---- Get default swapchain resources ----
    rhi::RenderPassHandle  defaultRP = renderer->GetDefaultRenderPass();
    rhi::FramebufferHandle defaultFB = renderer->GetDefaultFramebuffer();

    // ---- Main Loop ----
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Update surface size
        surface.UpdateSize();
        auto surfDesc = surface.GetDesc();

        if (surfDesc.Width == 0 || surfDesc.Height == 0)
            continue;

        // Begin frame
        renderer->BeginFrame();

        // Record clear-to-red
        rhi::ICommandBuffer& cmd = renderer->GetCommandBuffer();

        rhi::Rect2D renderArea;
        renderArea.Offset = {0, 0};
        renderArea.Extent = {surfDesc.Width, surfDesc.Height};

        rhi::ClearValue clearVal;
        clearVal.Color.RGBA = {1.0f, 0.0f, 0.0f, 1.0f};  // RED

        cmd.BeginRenderPass(defaultRP, defaultFB, renderArea, &clearVal, 1);
        cmd.EndRenderPass();

        // End frame (submit + present)
        renderer->EndFrame();
    }

    // ---- Cleanup ----
    renderer.reset();
    glfwDestroyWindow(window);
    glfwTerminate();

    printf("Sample exited cleanly.\n");
    return 0;
}
