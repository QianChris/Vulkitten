#pragma once

#include "Vulkitten/Core/Core.h"

#include <vector>

namespace Vulkitten {

class VulkanDevice;
class IWindow;

// ============================================================
// VkSwapchain — Vulkan swapchain + surface management.
//
// Creates VkSurfaceKHR from IWindow, manages swapchain lifecycle
// (create, acquire, present, recreate on resize). Owns per-image
// VkImageView resources.
// ============================================================
class VkSwapchain
{
public:
    VkSwapchain(VulkanDevice& device, IWindow& window);
    ~VkSwapchain();

    // Create or recreate swapchain (call on init and window resize).
    void Create(uint32_t width, uint32_t height);
    void Destroy();

    // Acquire next swapchain image. Returns true on success.
    bool AcquireNextImage(uint32_t& imageIndex, void* imageAvailableSemaphore);

    // Present the rendered image.
    bool Present(uint32_t imageIndex, void* renderFinishedSemaphore);

    uint32_t GetImageCount()   const { return m_ImageCount; }
    uint32_t GetCurrentWidth() const { return m_CurrentWidth; }
    uint32_t GetCurrentHeight()const { return m_CurrentHeight; }

    void* GetNativeSwapchain() const { return m_Swapchain; }
    void* GetNativeSurface()   const { return m_Surface; }

private:
    VulkanDevice& m_Device;
    IWindow&  m_Window;

    void* m_Surface = nullptr;     // VkSurfaceKHR
    void* m_Swapchain = nullptr;   // VkSwapchainKHR

    uint32_t m_ImageCount = 0;
    uint32_t m_CurrentWidth = 0;
    uint32_t m_CurrentHeight = 0;
};

} // namespace Vulkitten
