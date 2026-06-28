#pragma once

#include <cstdint>

namespace Vulkitten {

// ============================================================
// FrameContext - transient per-frame state.
//
// Owned by IDevice: IDevice::beginFrame() produces it,
// IDevice::endFrame() consumes it, and createCommandBuffer()
// uses it to allocate command buffers.
//
// The `internal` pointer carries backend-private data:
//   Vulkan:  pointer to per-frame resources (command pool,
//            fences, semaphores, command buffers).
//   OpenGL:  nullptr (GL is immediate-mode).
//
// Upper layers (RenderGraph, Passes) only read frameIndex
// and swapchainIndex. All synchronization details are
// managed internally by the backend.
// ============================================================

struct FrameContext
{
    // ---- Public (readable by upper layers) ----
    uint32_t FrameIndex = 0;        // Ring buffer index (0..framesInFlight-1)
    uint32_t SwapchainIndex = 0;    // Current swapchain image index

    // ---- Backend-Private ----
    void* Internal = nullptr;        // Backend-specific per-frame data
};

} // namespace Vulkitten
