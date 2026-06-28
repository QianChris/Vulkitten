#pragma once

#include "rhi/ISurface.hpp"
#include "rhi/Core/Types.hpp"

#include <vector>

// Forward-declare Vulkan types (no vulkan header in RHI header = iron rule)
// We use type-erased void* in the header, real types in .cpp

namespace rhi {

// ============================================================
// VKSwapchain — Vulkan swapchain wrapper
//
// Manages: VkSwapchainKHR, swapchain images, image views,
//          synchronization primitives for frame pacing.
// ============================================================

class VKSwapchain
{
public:
    VKSwapchain();
    ~VKSwapchain();

    // Create the swapchain from a surface
    bool Create(void* vkInstance, void* vkPhysicalDevice, void* vkDevice,
                ISurface* surface, uint32_t framesInFlight);

    // Destroy the swapchain (called on resize and shutdown)
    void Destroy();

    // Acquire next image index. Returns true on success.
    bool AcquireNextImage(uint32_t frameIndex, uint32_t* outImageIndex);

    // Submit + Present: submits the command buffer with proper sync, then presents.
    // Returns true on success, false if swapchain needs resize.
    bool SubmitAndPresent(uint32_t frameIndex, uint32_t imageIndex,
                          void* vkCommandBuffer, void* vkQueue);

    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }
    void* GetRenderPass() const { return m_RenderPass; }
    void* GetFramebuffer(uint32_t index) const;
    void* GetImageAvailableSemaphore(uint32_t frameIndex) const;
    void* GetRenderFinishedSemaphore(uint32_t frameIndex) const;
    void* GetInFlightFence(uint32_t frameIndex) const;

private:
    void CreateRenderPass();
    void CreateFramebuffers();
    void DestroyFramebuffers();

    // Vulkan handles (type-erased in header)
    void* m_Instance = nullptr;
    void* m_PhysicalDevice = nullptr;
    void* m_Device = nullptr;
    void* m_Surface = nullptr;
    void* m_Swapchain = nullptr;
    void* m_RenderPass = nullptr;

    // Swapchain images (owned by the swapchain)
    std::vector<void*> m_Images;
    std::vector<void*> m_ImageViews;
    std::vector<void*> m_Framebuffers;

    // Per-frame sync
    struct FrameSync
    {
        void* ImageAvailableSemaphore = nullptr;
        void* RenderFinishedSemaphore = nullptr;
        void* InFlightFence = nullptr;
    };
    std::vector<FrameSync> m_FrameSync;

    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
    uint32_t m_FramesInFlight = 2;
    bool     m_Initialized = false;
};

} // namespace rhi
