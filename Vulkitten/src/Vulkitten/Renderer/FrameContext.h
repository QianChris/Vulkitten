#pragma once

#include "Vulkitten/Core/Core.h"

#include <cstdint>
#include <vector>
#include <memory>

namespace Vulkitten {

// ============================================================
// Per-Frame Resource Pool
// ============================================================
// Holds temporary per-frame allocations that are recycled each
// frame (staging buffers, descriptor set arrays, etc.).
// For OpenGL this is a no-op; for Vulkan it manages VkBuffer
// and VkDescriptorSet pools.
struct PerFrameResourcePool
{
    // Reset all temporary allocations for the new frame.
    // Implementations should recycle, not free+alloc.
    void Reset() {}
};

// ============================================================
// FrameContext — per-frame render state.
//
// Created by IRenderer::BeginFrame() at the start of each frame.
// Carries frame metadata and API-specific synchronization
// primitives. The concrete backend (OpenGL / Vulkan) extends
// this with backend-specific data.
//
// OpenGL: most fields are no-ops (GL is immediate-mode).
// Vulkan: CommandPool, CommandBuffer, Fence, and Semaphore
//         are real Vulkan objects managed per-frame-in-flight.
// ============================================================
struct FrameContext
{
    // ---- Frame Identity ----

    uint32_t FrameIndex = 0;        // Monotonically increasing frame number
    uint32_t SwapchainIndex = 0;    // Current swapchain image index

    // ---- Command Recording ----

    void* CommandPool = nullptr;    // VkCommandPool (OpenGL: unused)
    void* CommandBuffer = nullptr;  // VkCommandBuffer (OpenGL: unused)

    // ---- Synchronization ----

    void* InFlightFence = nullptr;         // Signals when this frame's work is done
    void* ImageAvailableSemaphore = nullptr;   // Signals when swapchain image is ready
    void* RenderFinishedSemaphore = nullptr;   // Signals when rendering is complete

    // ---- Per-Frame Resources ----

    PerFrameResourcePool ResourcePool;
};

} // namespace Vulkitten
