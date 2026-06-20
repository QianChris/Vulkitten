#pragma once

#include "Vulkitten/Core/Core.h"

#include <cstdint>

namespace Vulkitten {

// Forward declarations (interfaces defined in their own headers)
class IDevice;
class IGpuResourceManager;
class RenderGraph;

// ============================================================
// RendererConfig — configuration for creating an IRenderer.
//
// Packs the three required backend dependencies. The concrete
// IRenderer implementation (OpenGL or Vulkan) is selected by
// the backend that creates these components.
// ============================================================
struct RendererConfig
{
    IDevice*              Device = nullptr;
    RenderGraph*          Graph = nullptr;
    IGpuResourceManager*  ResourceManager = nullptr;
};

// ============================================================
// IRenderer — the single backend renderer interface.
//
// The platform layer and scene layer interact with the renderer
// exclusively through this interface. Concrete implementations
// (e.g. OpenGL renderer, Vulkan renderer) are created by the
// application layer based on RendererConfig backend selection.
//
// Lifecycle:
//   Init() → BeginFrame() → [per-frame: Execute] → EndFrame() → Shutdown()
// ============================================================
class IRenderer
{
public:
    virtual ~IRenderer() = default;

    // ---- Lifecycle ----

    virtual void Init() = 0;
    virtual void Shutdown() = 0;

    // ---- Per-Frame ----

    // Called at the start of each frame. Creates the FrameContext
    // (acquires swapchain image for Vulkan, no-op for OpenGL).
    virtual void BeginFrame() = 0;

    // Execute all registered render passes via the RenderGraph.
    virtual void Execute() = 0;

    // Called at the end of each frame. Submits command buffers,
    // synchronizes (fence/semaphore), and presents the swapchain.
    virtual void EndFrame() = 0;

    // ---- Window Events ----

    // Handle window resize. Updates viewport, recreates
    // swapchain-dependent resources, and resizes all registered
    // framebuffers in the RenderGraph.
    virtual void OnWindowResize(uint32_t width, uint32_t height) = 0;

    // ---- Subsystem Access ----

    virtual IDevice&              GetDevice() = 0;
    virtual IGpuResourceManager&  GetResourceManager() = 0;
    virtual RenderGraph*          GetRenderGraph() = 0;
};

} // namespace Vulkitten
