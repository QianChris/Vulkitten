#include "vktpch.h"
#include "VkSwapchain.h"

#include "VulkanDevice.h"
#include "VulkanInstance.h"
#include "Vulkitten/Core/IWindow.h"
#include "Vulkitten/Core/ISurface.h"

#ifdef VKT_HAS_VULKAN
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include "Vulkitten/Perf/Instrumentor.h"

namespace Vulkitten {

VkSwapchain::VkSwapchain(VulkanDevice& device, VulkanInstance& instance, IWindow& window)
    : m_Device(device), m_Instance(instance), m_Window(window)
{
}

VkSwapchain::~VkSwapchain()
{
    Destroy();
#ifdef VKT_HAS_VULKAN
    // Surface is destroyed only at shutdown, not during resize
    auto vkInstance = static_cast<VkInstance>(m_Instance.GetNativeInstance());
    if (m_Surface && vkInstance)
    {
        vkDestroySurfaceKHR(vkInstance, static_cast<VkSurfaceKHR>(m_Surface), nullptr);
        m_Surface = nullptr;
    }
#endif
}

void VkSwapchain::Create(uint32_t width, uint32_t height)
{
#ifdef VKT_HAS_VULKAN
    CreateSurface();
    if (!m_Surface) { VKT_CORE_ERROR("VkSwapchain::Create - Surface creation failed"); return; }
    CreateSwapchain(width, height);
    if (!m_Swapchain) { VKT_CORE_ERROR("VkSwapchain::Create - Swapchain creation failed"); return; }
    CreateImageViews();
    if (m_ImageViews.empty()) { VKT_CORE_ERROR("VkSwapchain::Create - ImageView creation failed"); return; }
    CreateRenderPass();
    if (!m_RenderPass) { VKT_CORE_ERROR("VkSwapchain::Create - RenderPass creation failed"); return; }
    CreateFramebuffers();
    if (m_Framebuffers.empty()) { VKT_CORE_ERROR("VkSwapchain::Create - Framebuffer creation failed"); return; }
    VKT_CORE_INFO("VkSwapchain::Create - Complete {0}x{1}, {2} images", width, height, m_Framebuffers.size());
#endif
}

void VkSwapchain::Destroy()
{
#ifdef VKT_HAS_VULKAN
    auto vkDevice = static_cast<VkDevice>(m_Device.GetNativeDevice());
    if (vkDevice != VK_NULL_HANDLE)
    {
        // Must wait for device idle before destroying swapchain resources
        vkDeviceWaitIdle(vkDevice);

        for (auto fb : m_Framebuffers)
            if (fb) vkDestroyFramebuffer(vkDevice, static_cast<VkFramebuffer>(fb), nullptr);
        m_Framebuffers.clear();

        for (auto iv : m_ImageViews)
            if (iv) vkDestroyImageView(vkDevice, static_cast<VkImageView>(iv), nullptr);
        m_ImageViews.clear();

        if (m_RenderPass)
            vkDestroyRenderPass(vkDevice, static_cast<VkRenderPass>(m_RenderPass), nullptr);
        m_RenderPass = nullptr;

        if (m_Swapchain)
            vkDestroySwapchainKHR(vkDevice, static_cast<VkSwapchainKHR>(m_Swapchain), nullptr);
        m_Swapchain = nullptr;
    }
    // Surface is NOT destroyed here - it's owned by the window
    // and is only destroyed when the window is closed
#endif
}

void VkSwapchain::CreateSurface()
{
#ifdef VKT_HAS_VULKAN
    auto* surface = m_Window.GetSurface();
    if (!surface) return;
    GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(surface->GetNativeHandle());
    VkInstance vkInstance = static_cast<VkInstance>(m_Instance.GetNativeInstance());
    VkSurfaceKHR vkSurface;
    if (glfwCreateWindowSurface(vkInstance, glfwWindow, nullptr, &vkSurface) == VK_SUCCESS)
        m_Surface = vkSurface;
    else
        VKT_CORE_ERROR("VkSwapchain: Failed to create VkSurfaceKHR");
#endif
}

void VkSwapchain::CreateSwapchain(uint32_t w, uint32_t h)
{
#ifdef VKT_HAS_VULKAN
    auto vkPhysicalDevice = static_cast<VkPhysicalDevice>(m_Device.GetNativePhysicalDevice());
    auto vkDevice = static_cast<VkDevice>(m_Device.GetNativeDevice());
    auto vkSurface = static_cast<VkSurfaceKHR>(m_Surface);

    // Choose surface format
    VkSurfaceFormatKHR surfaceFormat{};
    surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
    surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, vkSurface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, vkSurface, &formatCount, formats.data());
    if (formatCount > 0 && !(formatCount == 1 && formats[0].format == VK_FORMAT_UNDEFINED))
        surfaceFormat = formats[0];
    m_Format = surfaceFormat.format;

    // Choose present mode
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR; // Always available

    // Surface capabilities
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkPhysicalDevice, vkSurface, &caps);
    if (caps.currentExtent.width != UINT32_MAX)
    {
        w = caps.currentExtent.width;
        h = caps.currentExtent.height;
    }
    // Clamp to valid range
    w = std::max(w, 1u);
    h = std::max(h, 1u);
    m_CurrentWidth = w;
    m_CurrentHeight = h;

    uint32_t imageCount = std::max(caps.minImageCount + 1, 2u);
    if (caps.maxImageCount > 0)
        imageCount = std::min(imageCount, caps.maxImageCount);
    m_ImageCount = imageCount;

    VkSwapchainCreateInfoKHR sci{};
    sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sci.surface = vkSurface;
    sci.minImageCount = imageCount;
    sci.imageFormat = surfaceFormat.format;
    sci.imageColorSpace = surfaceFormat.colorSpace;
    sci.imageExtent = {w, h};
    sci.imageArrayLayers = 1;
    sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sci.preTransform = caps.currentTransform;
    sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sci.presentMode = presentMode;
    sci.clipped = VK_TRUE;

    VkSwapchainKHR swapchain;
    if (vkCreateSwapchainKHR(vkDevice, &sci, nullptr, &swapchain) == VK_SUCCESS)
    {
        m_Swapchain = swapchain;
        VKT_CORE_INFO("VkSwapchain: Created {0}x{1} with {2} images", w, h, imageCount);
    }
#endif
}

void VkSwapchain::CreateImageViews()
{
#ifdef VKT_HAS_VULKAN
    auto vkDevice = static_cast<VkDevice>(m_Device.GetNativeDevice());
    auto vkSwapchain = static_cast<VkSwapchainKHR>(m_Swapchain);

    uint32_t count;
    vkGetSwapchainImagesKHR(vkDevice, vkSwapchain, &count, nullptr);
    m_Images.resize(count);
    m_ImageViews.resize(count);
    m_ImageCount = count;
    vkGetSwapchainImagesKHR(vkDevice, vkSwapchain, &count, reinterpret_cast<VkImage*>(m_Images.data()));

    for (uint32_t i = 0; i < count; i++)
    {
        VkImageViewCreateInfo iv{};
        iv.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        iv.image = static_cast<VkImage>(m_Images[i]);
        iv.viewType = VK_IMAGE_VIEW_TYPE_2D;
        iv.format = static_cast<VkFormat>(m_Format);
        iv.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        iv.subresourceRange.levelCount = 1;
        iv.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(vkDevice, &iv, nullptr, &imageView) == VK_SUCCESS)
            m_ImageViews[i] = imageView;
    }
#endif
}

void VkSwapchain::CreateRenderPass()
{
#ifdef VKT_HAS_VULKAN
    auto vkDevice = static_cast<VkDevice>(m_Device.GetNativeDevice());

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = static_cast<VkFormat>(m_Format);
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;

    VkRenderPassCreateInfo rp{};
    rp.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp.attachmentCount = 1;
    rp.pAttachments = &colorAttachment;
    rp.subpassCount = 1;
    rp.pSubpasses = &subpass;

    VkRenderPass renderPass;
    if (vkCreateRenderPass(vkDevice, &rp, nullptr, &renderPass) == VK_SUCCESS)
        m_RenderPass = renderPass;
#endif
}

void VkSwapchain::CreateFramebuffers()
{
#ifdef VKT_HAS_VULKAN
    auto vkDevice = static_cast<VkDevice>(m_Device.GetNativeDevice());
    auto vkRenderPass = static_cast<VkRenderPass>(m_RenderPass);

    m_Framebuffers.resize(m_ImageViews.size());
    for (uint32_t i = 0; i < m_ImageViews.size(); i++)
    {
        VkImageView attachments[] = {static_cast<VkImageView>(m_ImageViews[i])};

        VkFramebufferCreateInfo fb{};
        fb.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fb.renderPass = vkRenderPass;
        fb.attachmentCount = 1;
        fb.pAttachments = attachments;
        fb.width = m_CurrentWidth;
        fb.height = m_CurrentHeight;
        fb.layers = 1;

        VkFramebuffer framebuffer;
        if (vkCreateFramebuffer(vkDevice, &fb, nullptr, &framebuffer) == VK_SUCCESS)
            m_Framebuffers[i] = framebuffer;
    }
#endif
}

void* VkSwapchain::GetFramebuffer(uint32_t index) const
{
    if (index < m_Framebuffers.size()) return m_Framebuffers[index];
    return nullptr;
}

bool VkSwapchain::AcquireNextImage(uint32_t& imageIndex, uint64_t /*semaphore*/)
{
#ifdef VKT_HAS_VULKAN
    auto vkDevice = static_cast<VkDevice>(m_Device.GetNativeDevice());
    auto vkSwapchain = static_cast<VkSwapchainKHR>(m_Swapchain);
    if (!vkDevice || !vkSwapchain)
    {
        VKT_CORE_WARN("VkSwapchain::AcquireNextImage - device or swapchain is null");
        imageIndex = 0;
        return false;
    }
    uint32_t idx;
    if (vkAcquireNextImageKHR(vkDevice, vkSwapchain, UINT64_MAX, VK_NULL_HANDLE, VK_NULL_HANDLE, &idx) == VK_SUCCESS)
    {
        imageIndex = idx;
        return true;
    }
#endif
    imageIndex = 0;
    return false;
}

bool VkSwapchain::Present(uint32_t imageIndex, uint64_t /*semaphore*/)
{
#ifdef VKT_HAS_VULKAN
    auto vkDevice = static_cast<VkDevice>(m_Device.GetNativeDevice());
    // Get queue from device - we don't have a queue yet, placeholder
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    VkSwapchainKHR swapchain = static_cast<VkSwapchainKHR>(m_Swapchain);
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIndex;
    // TODO: get present queue from device
#endif
    return true;
}

} // namespace Vulkitten
