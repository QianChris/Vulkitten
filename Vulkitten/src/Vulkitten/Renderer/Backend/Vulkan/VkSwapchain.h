#pragma once

#include "Vulkitten/Core/Core.h"
#include <vector>

namespace Vulkitten {

class VulkanDevice;
class VulkanInstance;
class IWindow;

class VkSwapchain
{
public:
    VkSwapchain(VulkanDevice& device, VulkanInstance& instance, IWindow& window);
    ~VkSwapchain();

    void Create(uint32_t width, uint32_t height);
    void Destroy();

    bool AcquireNextImage(uint32_t& imageIndex, uint64_t imageAvailableSemaphore);
    bool Present(uint32_t imageIndex, uint64_t renderFinishedSemaphore);

    uint32_t GetImageCount() const { return m_ImageCount; }
    void* GetNativeSwapchain() const { return m_Swapchain; }
    void* GetNativeSurface() const { return m_Surface; }
    void* GetRenderPass() const { return m_RenderPass; }
    void* GetFramebuffer(uint32_t index) const;
    uint32_t GetWidth() const { return m_CurrentWidth; }
    uint32_t GetHeight() const { return m_CurrentHeight; }
    void* GetFormat() const { return reinterpret_cast<void*>(uint64_t(m_Format)); }

private:
    void CreateSurface();
    void CreateSwapchain(uint32_t w, uint32_t h);
    void CreateImageViews();
    void CreateFramebuffers();
    void CreateRenderPass();

    VulkanDevice&    m_Device;
    VulkanInstance&  m_Instance;
    IWindow&         m_Window;

    void* m_Surface = nullptr;
    void* m_Swapchain = nullptr;
    void* m_RenderPass = nullptr;
    uint32_t m_Format = 0;
    uint32_t m_ImageCount = 0;
    uint32_t m_CurrentWidth = 0;
    uint32_t m_CurrentHeight = 0;

    std::vector<void*> m_Images;
    std::vector<void*> m_ImageViews;
    std::vector<void*> m_Framebuffers;
};

} // namespace Vulkitten
