#pragma once

#include <cstdint>

namespace rhi {

// ============================================================
// FrameContext — transient per-frame state
//
// Produced by IRenderDevice::beginFrame(), consumed by endFrame().
// Upper layers only read frameIndex / swapchainIndex.
// All sync details (Fence / Semaphore / CommandPool) are
// managed internally by the backend via the `internal` pointer.
// ============================================================

struct FrameContext
{
    // ---- Public (readable by upper layers) ----
    uint32_t FrameIndex = 0;       // Ring buffer index (0..framesInFlight-1)
    uint32_t SwapchainIndex = 0;   // Current swapchain image index

    // ---- Backend-Private ----
    void* Internal = nullptr;      // Backend-specific per-frame data
};

} // namespace rhi
