#include "vktpch.h"
#include "VkSwapchain.h"

#include "VulkanDevice.h"
#include "Vulkitten/Core/IWindow.h"
#include "Vulkitten/Core/ISurface.h"

#ifdef VKT_HAS_VULKAN
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#endif

#include "Vulkitten/Perf/Instrumentor.h"

namespace Vulkitten {

VkSwapchain::VkSwapchain(VulkanDevice& device, IWindow& window)
    : m_Device(device), m_Window(window)
{
}

VkSwapchain::~VkSwapchain()
{
    Destroy();
}

void VkSwapchain::Create(uint32_t width, uint32_t height)
{
#ifdef VKT_HAS_VULKAN
    m_CurrentWidth = width;
    m_CurrentHeight = height;
    m_ImageCount = 2; // double-buffered default
    VKT_CORE_INFO("VkSwapchain: Created stub swapchain {0}x{1}", width, height);
#else
    m_CurrentWidth = width;
    m_CurrentHeight = height;
    m_ImageCount = 2;
#endif
}

void VkSwapchain::Destroy()
{
#ifdef VKT_HAS_VULKAN
    if (m_Swapchain)
    {
        vkDestroySwapchainKHR(
            static_cast<VkDevice>(m_Device.GetNativeDevice()),
            static_cast<VkSwapchainKHR>(m_Swapchain),
            nullptr);
        m_Swapchain = nullptr;
    }
    if (m_Surface)
    {
        // Need VkInstance to destroy surface; stored on VulkanInstance.
        // For stub, surface is nullptr so this is not reached.
        m_Surface = nullptr;
    }
#endif
}

bool VkSwapchain::AcquireNextImage(uint32_t& imageIndex, void* /*imageAvailableSemaphore*/)
{
    imageIndex = 0;
    return true;
}

bool VkSwapchain::Present(uint32_t /*imageIndex*/, void* /*renderFinishedSemaphore*/)
{
    return true;
}

} // namespace Vulkitten
