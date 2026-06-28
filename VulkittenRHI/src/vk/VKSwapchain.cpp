#include "VKSwapchain.hpp"

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>
#include <stdexcept>
#include <cstring>
#include <cstdio>

namespace rhi {

// ============================================================
// Vulkan helpers
// ============================================================

#define VK_CHECK(call) \
    do { \
        VkResult result = (call); \
        if (result != VK_SUCCESS) { \
            fprintf(stderr, "VK_CHECK failed: %s returned %d at %s:%d\n", \
                    #call, result, __FILE__, __LINE__); \
        } \
    } while(0)

// ============================================================
// Construction
// ============================================================

VKSwapchain::VKSwapchain() = default;

VKSwapchain::~VKSwapchain()
{
    Destroy();
}

// ============================================================
// Create
// ============================================================

bool VKSwapchain::Create(void* vkInstance, void* vkPhysicalDevice, void* vkDevice,
                          ISurface* surface, uint32_t framesInFlight)
{
    m_Instance = vkInstance;
    m_PhysicalDevice = vkPhysicalDevice;
    m_Device = vkDevice;
    m_FramesInFlight = framesInFlight;

    auto inst = static_cast<VkInstance>(m_Instance);
    auto physDev = static_cast<VkPhysicalDevice>(m_PhysicalDevice);
    auto dev = static_cast<VkDevice>(m_Device);

    // Create Vulkan surface from GLFW window
    GLFWwindow* window = static_cast<GLFWwindow*>(surface->GetNativeHandle());
    VkSurfaceKHR surfaceVK = VK_NULL_HANDLE;
    VkResult res = glfwCreateWindowSurface(inst, window, nullptr, &surfaceVK);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create Vulkan surface: %d\n", res);
        return false;
    }
    m_Surface = surfaceVK;

    auto surfDesc = surface->GetDesc();
    m_Width = surfDesc.Width;
    m_Height = surfDesc.Height;

    // Query surface capabilities
    VkSurfaceCapabilitiesKHR caps{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDev, surfaceVK, &caps);

    // Choose surface format
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physDev, surfaceVK, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physDev, surfaceVK, &formatCount, formats.data());

    VkSurfaceFormatKHR chosenFormat = formats[0];
    for (auto& f : formats)
    {
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            chosenFormat = f;
            break;
        }
    }

    // Choose present mode (FIFO = guaranteed)
    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physDev, surfaceVK, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physDev, surfaceVK, &presentModeCount, presentModes.data());

    VkPresentModeKHR chosenMode = VK_PRESENT_MODE_FIFO_KHR; // Guaranteed
    for (auto& m : presentModes)
    {
        if (m == VK_PRESENT_MODE_MAILBOX_KHR) { chosenMode = m; break; }
    }

    // Clamp extent
    if (caps.currentExtent.width != UINT32_MAX)
    {
        m_Width = caps.currentExtent.width;
        m_Height = caps.currentExtent.height;
    }

    uint32_t imageCount = std::max(caps.minImageCount + 1, framesInFlight);
    if (caps.maxImageCount > 0)
        imageCount = std::min(imageCount, caps.maxImageCount);

    // Create swapchain
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surfaceVK;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = chosenFormat.format;
    createInfo.imageColorSpace = chosenFormat.colorSpace;
    createInfo.imageExtent = {m_Width, m_Height};
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = caps.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = chosenMode;
    createInfo.clipped = VK_TRUE;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    res = vkCreateSwapchainKHR(dev, &createInfo, nullptr, &swapchain);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create swapchain: %d\n", res);
        return false;
    }
    m_Swapchain = swapchain;

    // Get swapchain images
    uint32_t actualImageCount = 0;
    vkGetSwapchainImagesKHR(dev, swapchain, &actualImageCount, nullptr);
    m_Images.resize(actualImageCount);
    vkGetSwapchainImagesKHR(dev, swapchain, &actualImageCount,
                            reinterpret_cast<VkImage*>(m_Images.data()));

    // Create image views
    m_ImageViews.resize(actualImageCount);
    for (uint32_t i = 0; i < actualImageCount; ++i)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = static_cast<VkImage>(m_Images[i]);
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = chosenFormat.format;
        viewInfo.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                               VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView = VK_NULL_HANDLE;
        res = vkCreateImageView(dev, &viewInfo, nullptr, &imageView);
        if (res != VK_SUCCESS)
        {
            fprintf(stderr, "Failed to create image view %u: %d\n", i, res);
            return false;
        }
        m_ImageViews[i] = imageView;
    }

    // Create per-frame sync primitives
    m_FrameSync.resize(framesInFlight);
    for (uint32_t i = 0; i < framesInFlight; ++i)
    {
        VkSemaphoreCreateInfo semInfo{};
        semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkSemaphore imageAvail = VK_NULL_HANDLE;
        VkSemaphore renderDone = VK_NULL_HANDLE;
        vkCreateSemaphore(dev, &semInfo, nullptr, &imageAvail);
        vkCreateSemaphore(dev, &semInfo, nullptr, &renderDone);

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VkFence fence = VK_NULL_HANDLE;
        vkCreateFence(dev, &fenceInfo, nullptr, &fence);

        m_FrameSync[i].ImageAvailableSemaphore = imageAvail;
        m_FrameSync[i].RenderFinishedSemaphore = renderDone;
        m_FrameSync[i].InFlightFence = fence;
    }

    // Create render pass (simple: one color attachment, clear)
    CreateRenderPass();

    // Create framebuffers (one per swapchain image)
    CreateFramebuffers();

    m_Initialized = true;
    return true;
}

// ============================================================
// Destroy
// ============================================================

void VKSwapchain::Destroy()
{
    auto dev = static_cast<VkDevice>(m_Device);

    if (dev)
    {
        vkDeviceWaitIdle(dev);

        // Destroy framebuffers
        DestroyFramebuffers();

        // Destroy render pass
        if (m_RenderPass)
        {
            vkDestroyRenderPass(dev, static_cast<VkRenderPass>(m_RenderPass), nullptr);
            m_RenderPass = nullptr;
        }

        // Destroy sync objects
        for (auto& fs : m_FrameSync)
        {
            if (fs.ImageAvailableSemaphore)
                vkDestroySemaphore(dev, static_cast<VkSemaphore>(fs.ImageAvailableSemaphore), nullptr);
            if (fs.RenderFinishedSemaphore)
                vkDestroySemaphore(dev, static_cast<VkSemaphore>(fs.RenderFinishedSemaphore), nullptr);
            if (fs.InFlightFence)
                vkDestroyFence(dev, static_cast<VkFence>(fs.InFlightFence), nullptr);
        }
        m_FrameSync.clear();

        // Destroy image views
        for (auto& iv : m_ImageViews)
        {
            if (iv)
                vkDestroyImageView(dev, static_cast<VkImageView>(iv), nullptr);
        }
        m_ImageViews.clear();
        m_Images.clear();

        // Destroy swapchain
        if (m_Swapchain)
        {
            vkDestroySwapchainKHR(dev, static_cast<VkSwapchainKHR>(m_Swapchain), nullptr);
            m_Swapchain = nullptr;
        }

        // Destroy surface
        if (m_Surface && m_Instance)
        {
            vkDestroySurfaceKHR(static_cast<VkInstance>(m_Instance),
                                static_cast<VkSurfaceKHR>(m_Surface), nullptr);
            m_Surface = nullptr;
        }
    }

    m_Initialized = false;
}

// ============================================================
// Acquire / Present
// ============================================================

bool VKSwapchain::AcquireNextImage(uint32_t frameIndex, uint32_t* outImageIndex)
{
    auto dev = static_cast<VkDevice>(m_Device);
    auto& fs = m_FrameSync[frameIndex];

    // Wait for previous frame's fence
    vkWaitForFences(dev, 1, reinterpret_cast<VkFence*>(&fs.InFlightFence), VK_TRUE, UINT64_MAX);
    vkResetFences(dev, 1, reinterpret_cast<VkFence*>(&fs.InFlightFence));

    // Acquire next swapchain image
    uint32_t imageIndex = 0;
    VkResult res = vkAcquireNextImageKHR(dev,
        static_cast<VkSwapchainKHR>(m_Swapchain), UINT64_MAX,
        static_cast<VkSemaphore>(fs.ImageAvailableSemaphore),
        VK_NULL_HANDLE, &imageIndex);

    if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
    {
        return false; // Needs resize
    }
    else if (res != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to acquire swapchain image: %d\n", res);
        return false;
    }

    *outImageIndex = imageIndex;
    return true;
}

bool VKSwapchain::SubmitAndPresent(uint32_t frameIndex, uint32_t imageIndex,
                                    void* vkCommandBuffer, void* vkQueue)
{
    auto dev = static_cast<VkDevice>(m_Device);
    auto queue = static_cast<VkQueue>(vkQueue);
    auto cmd = static_cast<VkCommandBuffer>(vkCommandBuffer);
    auto& fs = m_FrameSync[frameIndex];

    // Submit: wait on image-available, signal render-finished
    VkSemaphore waitSemaphores[] = {static_cast<VkSemaphore>(fs.ImageAvailableSemaphore)};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {static_cast<VkSemaphore>(fs.RenderFinishedSemaphore)};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    VkResult res = vkQueueSubmit(queue, 1, &submitInfo,
                                  static_cast<VkFence>(fs.InFlightFence));
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to submit: %d\n", res);
        return false;
    }

    // Present: wait on render-finished
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;

    VkSwapchainKHR sc = static_cast<VkSwapchainKHR>(m_Swapchain);
    presentInfo.pSwapchains = &sc;
    presentInfo.pImageIndices = &imageIndex;

    res = vkQueuePresentKHR(queue, &presentInfo);

    if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
        return false;
    else if (res != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to present: %d\n", res);
        return false;
    }

    return true;
}

void* VKSwapchain::GetFramebuffer(uint32_t index) const
{
    if (index < m_Framebuffers.size())
        return m_Framebuffers[index];
    return nullptr;
}

void* VKSwapchain::GetImageAvailableSemaphore(uint32_t frameIndex) const
{
    if (frameIndex < m_FrameSync.size())
        return const_cast<void*>(m_FrameSync[frameIndex].ImageAvailableSemaphore);
    return nullptr;
}

void* VKSwapchain::GetRenderFinishedSemaphore(uint32_t frameIndex) const
{
    if (frameIndex < m_FrameSync.size())
        return const_cast<void*>(m_FrameSync[frameIndex].RenderFinishedSemaphore);
    return nullptr;
}

void* VKSwapchain::GetInFlightFence(uint32_t frameIndex) const
{
    if (frameIndex < m_FrameSync.size())
        return const_cast<void*>(m_FrameSync[frameIndex].InFlightFence);
    return nullptr;
}

// ============================================================
// RenderPass & Framebuffers (internal)
// ============================================================

void VKSwapchain::CreateRenderPass()
{
    auto dev = static_cast<VkDevice>(m_Device);

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = VK_FORMAT_B8G8R8A8_UNORM;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkRenderPass rp = VK_NULL_HANDLE;
    VkResult res = vkCreateRenderPass(dev, &renderPassInfo, nullptr, &rp);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create render pass: %d\n", res);
        return;
    }
    m_RenderPass = rp;
}

void VKSwapchain::CreateFramebuffers()
{
    auto dev = static_cast<VkDevice>(m_Device);

    m_Framebuffers.resize(m_ImageViews.size());
    for (size_t i = 0; i < m_ImageViews.size(); ++i)
    {
        VkImageView attachments[] = {static_cast<VkImageView>(m_ImageViews[i])};

        VkFramebufferCreateInfo fbInfo{};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = static_cast<VkRenderPass>(m_RenderPass);
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = attachments;
        fbInfo.width = m_Width;
        fbInfo.height = m_Height;
        fbInfo.layers = 1;

        VkFramebuffer fb = VK_NULL_HANDLE;
        VkResult res = vkCreateFramebuffer(dev, &fbInfo, nullptr, &fb);
        if (res != VK_SUCCESS)
        {
            fprintf(stderr, "Failed to create framebuffer %zu: %d\n", i, res);
            return;
        }
        m_Framebuffers[i] = fb;
    }
}

void VKSwapchain::DestroyFramebuffers()
{
    auto dev = static_cast<VkDevice>(m_Device);
    for (auto& fb : m_Framebuffers)
    {
        if (fb)
            vkDestroyFramebuffer(dev, static_cast<VkFramebuffer>(fb), nullptr);
    }
    m_Framebuffers.clear();
}

} // namespace rhi
