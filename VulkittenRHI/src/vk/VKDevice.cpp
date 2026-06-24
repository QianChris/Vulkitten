#include "VKDevice.hpp"
#include "VKCommandBuffer.hpp"

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>
#include <stdexcept>
#include <cstring>
#include <cstdio>
#include <vector>

namespace rhi {

// ============================================================
// Helper macros
// ============================================================

#define VK_CHECK(call) \
    do { \
        VkResult result = (call); \
        if (result != VK_SUCCESS) { \
            fprintf(stderr, "VK_CHECK failed: %s returned %d at %s:%d\n", \
                    #call, result, __FILE__, __LINE__); \
        } \
    } while(0)

#ifndef NDEBUG
#define VK_CHECK_FATAL(call, msg) \
    do { \
        VkResult result = (call); \
        if (result != VK_SUCCESS) { \
            throw std::runtime_error(msg); \
        } \
    } while(0)
#else
#define VK_CHECK_FATAL VK_CHECK
#endif

// ============================================================
// Construction
// ============================================================

VKDevice::VKDevice(ISurface* surface)
    : m_Surface(surface)
{
    // Reserve slot 0 as null
    m_Slots.push_back({0, 0, 1, false});
}

VKDevice::~VKDevice()
{
    Shutdown();
}

// ============================================================
// Init / Shutdown
// ============================================================

void VKDevice::Init()
{
    if (!m_Surface)
        throw std::runtime_error("VKDevice: no surface provided");

    // === Create Vulkan Instance ===
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "VulkittenRHI Sample";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "VulkittenRHI";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    // Validation layers
    std::vector<const char*> layers;
#ifndef NDEBUG
    layers.push_back("VK_LAYER_KHRONOS_validation");
#endif

    // Instance extensions
    uint32_t glfwExtCount = 0;
    const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);
    std::vector<const char*> extensions(glfwExts, glfwExts + glfwExtCount);
#ifndef NDEBUG
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    VkInstanceCreateInfo instInfo{};
    instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instInfo.pApplicationInfo = &appInfo;
    instInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
    instInfo.ppEnabledLayerNames = layers.data();
    instInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    instInfo.ppEnabledExtensionNames = extensions.data();

    VkInstance instance = VK_NULL_HANDLE;
    VkResult res = vkCreateInstance(&instInfo, nullptr, &instance);
    if (res != VK_SUCCESS)
        throw std::runtime_error("VKDevice: failed to create Vulkan instance");
    m_Instance = instance;

    // Load instance-level functions
    // Vulkan functions loaded directly via vulkan-1.lib loader

    // === Pick Physical Device ===
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0)
        throw std::runtime_error("VKDevice: no Vulkan-capable GPU found");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    // Pick the first discrete GPU, or fall back to the first device
    VkPhysicalDevice chosenDevice = devices[0];
    for (auto& d : devices)
    {
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(d, &props);
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            chosenDevice = d;
            break;
        }
    }
    m_PhysicalDevice = chosenDevice;

    // === Find Queue Families ===
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(chosenDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(chosenDevice, &queueFamilyCount, queueFamilies.data());

    // Create surface for queue family check
    GLFWwindow* window = static_cast<GLFWwindow*>(m_Surface->GetNativeHandle());
    VkSurfaceKHR tempSurface = VK_NULL_HANDLE;
    glfwCreateWindowSurface(instance, window, nullptr, &tempSurface);

    bool foundGraphics = false;
    for (uint32_t i = 0; i < queueFamilyCount; ++i)
    {
        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(chosenDevice, i, tempSurface, &presentSupport);

        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && presentSupport)
        {
            m_GraphicsQueueFamily = i;
            m_PresentQueueFamily = i;
            foundGraphics = true;
            break;
        }
    }

    // Destroy temp surface
    vkDestroySurfaceKHR(instance, tempSurface, nullptr);

    if (!foundGraphics)
        throw std::runtime_error("VKDevice: no suitable graphics+present queue family");

    // === Create Logical Device ===
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = m_GraphicsQueueFamily;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &queuePriority;

    const char* deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.enabledExtensionCount = 1;
    deviceInfo.ppEnabledExtensionNames = deviceExtensions;
    deviceInfo.pEnabledFeatures = &deviceFeatures;

    VkDevice device = VK_NULL_HANDLE;
    res = vkCreateDevice(chosenDevice, &deviceInfo, nullptr, &device);
    if (res != VK_SUCCESS)
        throw std::runtime_error("VKDevice: failed to create logical device");
    m_Device = device;

    // Load device-level functions
    // Device-level Vulkan functions loaded directly via vulkan-1.lib loader

    // Get queues
    vkGetDeviceQueue(device, m_GraphicsQueueFamily, 0,
                     reinterpret_cast<VkQueue*>(&m_GraphicsQueue));
    vkGetDeviceQueue(device, m_PresentQueueFamily, 0,
                     reinterpret_cast<VkQueue*>(&m_PresentQueue));

    m_FramesInFlight = 2;

    // === Create Swapchain ===
    m_Swapchain = std::make_unique<VKSwapchain>();
    if (!m_Swapchain->Create(m_Instance, m_PhysicalDevice, m_Device,
                              m_Surface, m_FramesInFlight))
    {
        throw std::runtime_error("VKDevice: failed to create swapchain");
    }

    // === Create Command Pools ===
    CreateCommandPools();

    m_Initialized = true;
}

void VKDevice::Shutdown()
{
    if (!m_Initialized)
        return;

    auto dev = static_cast<VkDevice>(m_Device);
    vkDeviceWaitIdle(dev);

    DestroyCommandPools();
    m_Swapchain.reset();

    if (m_Device)
    {
        vkDestroyDevice(dev, nullptr);
        m_Device = nullptr;
    }

    if (m_Instance)
    {
        vkDestroyInstance(static_cast<VkInstance>(m_Instance), nullptr);
        m_Instance = nullptr;
    }

    m_Initialized = false;
}

// ============================================================
// Frame Lifecycle
// ============================================================

FrameContext VKDevice::BeginFrame()
{
    // Detect surface size change (e.g., window was enlarged — swapchain
    // won't report OUT_OF_DATE for this, so we check explicitly)
    auto surfDesc = m_Surface->GetDesc();
    if (surfDesc.Width != m_Swapchain->GetWidth() ||
        surfDesc.Height != m_Swapchain->GetHeight())
    {
        OnResize(surfDesc.Width, surfDesc.Height);
    }

    uint32_t imageIndex = 0;
    if (!m_Swapchain->AcquireNextImage(m_FrameIndex, &imageIndex))
    {
        // Still out of date (platform-specific, e.g. minimized)
        OnResize(m_Swapchain->GetWidth(), m_Swapchain->GetHeight());
        m_Swapchain->AcquireNextImage(m_FrameIndex, &imageIndex);
    }

    m_CurrentImageIndex = imageIndex;

    FrameContext ctx;
    ctx.FrameIndex = m_FrameIndex;
    ctx.SwapchainIndex = imageIndex;
    return ctx;
}

void VKDevice::EndFrame(FrameContext ctx)
{
    // Submit + Present with proper sync (image-available → submit → render-finished → present)
    // The VkCommandBuffer was already ended by VKCommandBuffer::End().
    // SubmitAndPresent handles: wait on image-available semaphore,
    // submit the command buffer, signal render-finished semaphore,
    // signal the in-flight fence, then present waiting on render-finished.

    if (!m_Swapchain->SubmitAndPresent(ctx.FrameIndex, ctx.SwapchainIndex,
                                        m_CurrentVkCmd, m_GraphicsQueue))
    {
        OnResize(m_Swapchain->GetWidth(), m_Swapchain->GetHeight());
    }

    m_CurrentVkCmd = nullptr;
    m_FrameIndex = (m_FrameIndex + 1) % m_FramesInFlight;
}

// ============================================================
// Command Buffer
// ============================================================

std::unique_ptr<ICommandBuffer> VKDevice::CreateCommandBuffer(
    FrameContext ctx, CommandBufferLevel /*level*/)
{
    auto dev = static_cast<VkDevice>(m_Device);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = static_cast<VkCommandPool>(GetCommandPool(ctx.FrameIndex));
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VK_CHECK(vkAllocateCommandBuffers(dev, &allocInfo, &cmd));

    // Store for EndFrame submission
    m_CurrentVkCmd = cmd;

    return std::make_unique<VKCommandBuffer>(*this, cmd);
}

// ============================================================
// Resource Creation (stubs for MVP)
// ============================================================

BufferHandle VKDevice::CreateBuffer(const BufferDesc& /*desc*/, const void* /*initialData*/)
{
    return BufferHandle{};
}

TextureHandle VKDevice::CreateTexture(const TextureDesc& /*desc*/, const void* /*initialData*/)
{
    return TextureHandle{};
}

ShaderHandle VKDevice::CreateShader(ShaderStage /*stage*/, const ShaderBytecode& /*bytecode*/)
{
    return ShaderHandle{};
}

PipelineHandle VKDevice::CreatePipeline(const PipelineDesc& /*desc*/)
{
    return PipelineHandle{};
}

GeometryHandle VKDevice::CreateGeometry(const GeometryDesc& /*desc*/)
{
    return GeometryHandle{};
}

SamplerHandle VKDevice::CreateSampler(const SamplerDesc& /*desc*/)
{
    return SamplerHandle{};
}

RenderPassHandle VKDevice::CreateRenderPass(const RenderPassDesc& /*desc*/)
{
    // For MVP, use the swapchain's built-in render pass
    // Track the slot so we can refresh it on resize
    m_DefaultRenderPassSlotId = FindFreeSlot();
    m_Slots[m_DefaultRenderPassSlotId].Alive = true;
    m_Slots[m_DefaultRenderPassSlotId].GpuHandle = reinterpret_cast<uint64_t>(m_Swapchain->GetRenderPass());
    uint32_t gen = m_Slots[m_DefaultRenderPassSlotId].Generation;
    return RenderPassHandle{m_DefaultRenderPassSlotId, gen};
}

FramebufferHandle VKDevice::CreateFramebuffer(const FramebufferDesc& /*desc*/)
{
    uint32_t id = FindFreeSlot();
    m_Slots[id].Alive = true;
    // Sentinel: resolved to the correct per-image VkFramebuffer at BeginRenderPass time
    m_Slots[id].GpuHandle = kSwapchainFramebufferSentinel;
    uint32_t gen = m_Slots[id].Generation;
    return FramebufferHandle{id, gen};
}

// ============================================================
// Window Events
// ============================================================

void VKDevice::OnResize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
        return;

    auto dev = static_cast<VkDevice>(m_Device);
    vkDeviceWaitIdle(dev);

    m_Swapchain->Destroy();

    if (!m_Swapchain->Create(m_Instance, m_PhysicalDevice, m_Device,
                              m_Surface, m_FramesInFlight))
    {
        fprintf(stderr, "VKDevice: failed to recreate swapchain on resize\n");
        return;
    }

    // Refresh the default render pass handle — Destroy() freed the old
    // VkRenderPass, Create() made a new one. Update the slot to point to it.
    if (m_DefaultRenderPassSlotId > 0 && m_DefaultRenderPassSlotId < m_Slots.size())
    {
        m_Slots[m_DefaultRenderPassSlotId].GpuHandle =
            reinterpret_cast<uint64_t>(m_Swapchain->GetRenderPass());
    }
}

void VKDevice::WaitIdle()
{
    if (m_Device)
        vkDeviceWaitIdle(static_cast<VkDevice>(m_Device));
}

// ============================================================
// Internal: Handle Pool
// ============================================================

VKDevice::GpuSlot* VKDevice::GetSlot(uint32_t id)
{
    if (id == 0 || id >= m_Slots.size())
        return nullptr;
    return &m_Slots[id];
}

uint32_t VKDevice::FindFreeSlot()
{
    if (!m_FreeIndices.empty())
    {
        uint32_t idx = m_FreeIndices.back();
        m_FreeIndices.pop_back();
        m_Slots[idx].Generation++;
        return idx;
    }

    uint32_t idx = static_cast<uint32_t>(m_Slots.size());
    m_Slots.push_back({0, 0, 1, false});
    return idx;
}

uint32_t VKDevice::FindMemoryType(uint32_t typeFilter, uint32_t properties)
{
    VkPhysicalDeviceMemoryProperties memProps{};
    vkGetPhysicalDeviceMemoryProperties(static_cast<VkPhysicalDevice>(m_PhysicalDevice), &memProps);

    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i)
    {
        if ((typeFilter & (1 << i)) &&
            (memProps.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }

    return 0;
}

// ============================================================
// Command Pools
// ============================================================

void VKDevice::CreateCommandPools()
{
    auto dev = static_cast<VkDevice>(m_Device);

    m_CommandPools.resize(m_FramesInFlight);
    for (uint32_t i = 0; i < m_FramesInFlight; ++i)
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = m_GraphicsQueueFamily;

        VkCommandPool pool = VK_NULL_HANDLE;
        VK_CHECK(vkCreateCommandPool(dev, &poolInfo, nullptr, &pool));
        m_CommandPools[i] = pool;
    }
}

void VKDevice::DestroyCommandPools()
{
    auto dev = static_cast<VkDevice>(m_Device);
    for (auto& pool : m_CommandPools)
    {
        if (pool)
            vkDestroyCommandPool(dev, static_cast<VkCommandPool>(pool), nullptr);
    }
    m_CommandPools.clear();
}

void* VKDevice::GetCommandPool(uint32_t frameIndex) const
{
    if (frameIndex < m_CommandPools.size())
        return m_CommandPools[frameIndex];
    return nullptr;
}

void* VKDevice::GetSwapchainFramebuffer(uint32_t imageIndex) const
{
    if (m_Swapchain)
        return m_Swapchain->GetFramebuffer(imageIndex);
    return nullptr;
}

} // namespace rhi
