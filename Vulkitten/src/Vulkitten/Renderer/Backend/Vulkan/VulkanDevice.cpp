#include "vktpch.h"
#include "VulkanDevice.h"

#include "VulkanInstance.h"
#include "Vulkitten/Core/ClassFactory.h"
#include "Vulkitten/Renderer/FrameContext.h"

#ifdef VKT_HAS_VULKAN
#include <vulkan/vulkan.h>
#endif

#include "Vulkitten/Perf/Instrumentor.h"

namespace Vulkitten {

VulkanDevice::VulkanDevice(VulkanInstance& instance)
    : m_Instance(instance)
{
}

VulkanDevice::~VulkanDevice()
{
    Shutdown();
}

void VulkanDevice::Init()
{
    VKT_PROFILE_FUNCTION();

#ifdef VKT_HAS_VULKAN
    auto vkInstance = static_cast<VkInstance>(m_Instance.GetNativeInstance());
    if (!vkInstance) return;

    // Enumerate physical devices
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr);
    if (deviceCount == 0)
    {
        VKT_CORE_WARN("VulkanDevice: No Vulkan-capable physical devices found");
        return;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices.data());

    // Prefer discrete GPU
    VkPhysicalDevice selectedDevice = devices[0];
    for (auto& device : devices)
    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(device, &props);
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            selectedDevice = device;
            VKT_CORE_INFO("VulkanDevice: Selected discrete GPU: {0}", props.deviceName);
            break;
        }
    }
    m_PhysicalDevice = selectedDevice;

    // Query queue families (simple: find first graphics-capable queue)
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(selectedDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(selectedDevice, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilyCount; i++)
    {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            m_GraphicsQueueFamily = i;
            m_PresentQueueFamily = i;
            m_TransferQueueFamily = i;
            break;
        }
    }

    // Create logical device with swapchain extension
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = m_GraphicsQueueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.enabledExtensionCount = 1;
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

    VkDevice vkDevice;
    VkResult result = vkCreateDevice(selectedDevice, &deviceCreateInfo, nullptr, &vkDevice);
    if (result == VK_SUCCESS)
    {
        m_NativeDevice = vkDevice;
        VKT_CORE_INFO("VulkanDevice: Logical device created with swapchain support");
    }
    else
    {
        VKT_CORE_WARN("VulkanDevice: vkCreateDevice failed with result {0}", static_cast<int>(result));
    }
#else
    VKT_CORE_INFO("VulkanDevice: Vulkan SDK not available — device is a stub");
#endif
}

void VulkanDevice::Shutdown()
{
#ifdef VKT_HAS_VULKAN
    if (m_NativeDevice)
    {
        vkDestroyDevice(static_cast<VkDevice>(m_NativeDevice), nullptr);
        m_NativeDevice = nullptr;
    }
    m_PhysicalDevice = nullptr;
#endif
}

// ---- Frame Lifecycle ----

FrameContext VulkanDevice::beginFrame()
{
    // [HACK: 抽象层缺 Vulkan beginFrame — Task 15 实现]
    return FrameContext{};
}

void VulkanDevice::endFrame(FrameContext /*ctx*/)
{
    // [HACK: 抽象层缺 Vulkan endFrame — Task 15 实现]
}

// ---- Command Buffer (stub until Task 16) ----

rhi::ICommandBuffer* VulkanDevice::createCommandBuffer(FrameContext /*ctx*/)
{
    // [HACK: 抽象层缺 VkCommandBuffer — Task 16 创建]
    return nullptr;
}

// ---- Resource Creation (stubs until Task 15) ----

rhi::BufferHandle VulkanDevice::createBuffer(const rhi::BufferDesc& /*desc*/, const void* /*initialData*/)
{
    // [HACK: 抽象层缺 Vk Buffer 创建 — Task 15 实现]
    return {};
}

rhi::TextureHandle VulkanDevice::createTexture(const rhi::TextureDesc& /*desc*/, const void* /*initialData*/)
{
    // [HACK: 抽象层缺 Vk Texture 创建 — Task 15 实现]
    return {};
}

rhi::ShaderHandle VulkanDevice::createShader(rhi::ShaderStage /*stage*/, const ShaderBytecode& /*bytecode*/)
{
    // [HACK: 抽象层缺 Vk Shader 创建 — Task 15 实现]
    return {};
}

rhi::PipelineHandle VulkanDevice::createPipeline(const rhi::PipelineDesc& /*desc*/)
{
    // [HACK: 抽象层缺 Vk Pipeline 创建 — Task 15 实现]
    return {};
}

rhi::GeometryHandle VulkanDevice::createGeometry(const rhi::GeometryDesc& /*desc*/)
{
    // [HACK: 抽象层缺 Vk Geometry 创建 — Task 15 实现]
    return {};
}

rhi::SamplerHandle VulkanDevice::createSampler(const rhi::SamplerDesc& /*desc*/)
{
    // [HACK: 抽象层缺 Vk Sampler 创建 — Task 15 实现]
    return {};
}

rhi::RenderPassHandle VulkanDevice::createRenderPass(const rhi::RenderPassDesc& /*desc*/)
{
    // [HACK: 抽象层缺 Vk RenderPass 创建 — Task 15 实现]
    return {};
}

rhi::FramebufferHandle VulkanDevice::createFramebuffer(const rhi::FramebufferDesc& /*desc*/)
{
    // [HACK: 抽象层缺 Vk Framebuffer 创建 — Task 15 实现]
    return {};
}

// ---- Window ----

void VulkanDevice::onResize(uint32_t /*width*/, uint32_t /*height*/)
{
    // [HACK: 抽象层缺 Vulkan swapchain resize — Task 15 实现]
}

// ---- Utilities ----

void VulkanDevice::waitIdle()
{
#ifdef VKT_HAS_VULKAN
    if (m_NativeDevice)
        vkDeviceWaitIdle(static_cast<VkDevice>(m_NativeDevice));
#endif
}

// ---- Legacy ----

void VulkanDevice::Submit(FrameContext& /*frameContext*/)
{
    // Stub: vkQueueSubmit + vkQueuePresentKHR
    // [HACK: 抽象层缺完整 submit/present — Task 15 实现]
}

} // namespace Vulkitten
