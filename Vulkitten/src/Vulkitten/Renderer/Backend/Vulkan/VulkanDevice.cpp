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
    m_Slots.emplace_back(); // Reserve slot 0 as null
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

rhi::BufferHandle VulkanDevice::createBuffer(const rhi::BufferDesc& desc, const void* initialData)
{
#ifdef VKT_HAS_VULKAN
    auto vkDevice = static_cast<VkDevice>(m_NativeDevice);
    if (!vkDevice) return {};

    VkBufferUsageFlags usage = 0;
    if (rhi::HasUsage(desc.Usage, rhi::BufferUsage::Vertex))   usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (rhi::HasUsage(desc.Usage, rhi::BufferUsage::Index))    usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (rhi::HasUsage(desc.Usage, rhi::BufferUsage::Uniform))  usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (rhi::HasUsage(desc.Usage, rhi::BufferUsage::Storage))  usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    if (rhi::HasUsage(desc.Usage, rhi::BufferUsage::TransferSrc)) usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    if (rhi::HasUsage(desc.Usage, rhi::BufferUsage::TransferDst)) usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = desc.Size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer vkBuf;
    if (vkCreateBuffer(vkDevice, &bufferInfo, nullptr, &vkBuf) != VK_SUCCESS)
        return {};

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(vkDevice, vkBuf, &memReqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memReqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkDeviceMemory memory;
    if (vkAllocateMemory(vkDevice, &allocInfo, nullptr, &memory) != VK_SUCCESS)
    {
        vkDestroyBuffer(vkDevice, vkBuf, nullptr);
        return {};
    }
    vkBindBufferMemory(vkDevice, vkBuf, memory, 0);

    // Upload initial data if provided
    if (initialData)
    {
        void* mapped;
        vkMapMemory(vkDevice, memory, 0, desc.Size, 0, &mapped);
        memcpy(mapped, initialData, static_cast<size_t>(desc.Size));
        vkUnmapMemory(vkDevice, memory);
    }

    auto handle = AllocHandle<rhi::BufferTag>();
    GpuSlot* slot = GetSlot(handle.GetId());
    if (slot)
    {
        slot->GpuHandle = reinterpret_cast<uint64_t>(vkBuf);
        slot->GpuHandle2 = reinterpret_cast<uint64_t>(memory);
    }
    return handle;
#else
    (void)desc; (void)initialData;
    return {};
#endif
}

rhi::TextureHandle VulkanDevice::createTexture(const rhi::TextureDesc& desc, const void* initialData)
{
#ifdef VKT_HAS_VULKAN
    auto vkDevice = static_cast<VkDevice>(m_NativeDevice);
    if (!vkDevice) return {};

    VkFormat vkFmt = VK_FORMAT_R8G8B8A8_UNORM; // TODO: full format mapping
    VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (rhi::HasUsage(desc.Usage, rhi::TextureUsage::Sampled))
        usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if (rhi::HasUsage(desc.Usage, rhi::TextureUsage::ColorAttachment))
        usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (rhi::HasUsage(desc.Usage, rhi::TextureUsage::DepthStencilAttachment))
        usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = vkFmt;
    imageInfo.extent = {desc.Extent.Width, desc.Extent.Height, 1};
    imageInfo.mipLevels = desc.MipLevels;
    imageInfo.arrayLayers = desc.ArrayLayers;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkImage vkImage;
    if (vkCreateImage(vkDevice, &imageInfo, nullptr, &vkImage) != VK_SUCCESS)
        return {};

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(vkDevice, vkImage, &memReqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memReqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkDeviceMemory memory;
    if (vkAllocateMemory(vkDevice, &allocInfo, nullptr, &memory) != VK_SUCCESS)
    {
        vkDestroyImage(vkDevice, vkImage, nullptr);
        return {};
    }
    vkBindImageMemory(vkDevice, vkImage, memory, 0);

    // Create image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = vkImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = vkFmt;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = desc.MipLevels;
    viewInfo.subresourceRange.layerCount = desc.ArrayLayers;

    VkImageView vkView;
    if (vkCreateImageView(vkDevice, &viewInfo, nullptr, &vkView) != VK_SUCCESS)
    {
        vkDestroyImage(vkDevice, vkImage, nullptr);
        vkFreeMemory(vkDevice, memory, nullptr);
        return {};
    }

    auto handle = AllocHandle<rhi::TextureTag>();
    GpuSlot* slot = GetSlot(handle.GetId());
    if (slot)
    {
        slot->GpuHandle = reinterpret_cast<uint64_t>(vkImage);
        slot->GpuHandle2 = reinterpret_cast<uint64_t>(vkView);
    }
    (void)initialData;
    return handle;
#else
    (void)desc; (void)initialData;
    return {};
#endif
}

rhi::ShaderHandle VulkanDevice::createShader(rhi::ShaderStage /*stage*/, const ShaderBytecode& bytecode)
{
#ifdef VKT_HAS_VULKAN
    auto vkDevice = static_cast<VkDevice>(m_NativeDevice);
    if (!vkDevice || !bytecode.Data) return {};

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = bytecode.Size;
    createInfo.pCode = reinterpret_cast<const uint32_t*>(bytecode.Data);

    VkShaderModule module;
    if (vkCreateShaderModule(vkDevice, &createInfo, nullptr, &module) != VK_SUCCESS)
        return {};

    auto handle = AllocHandle<rhi::ShaderTag>();
    GpuSlot* slot = GetSlot(handle.GetId());
    if (slot) slot->GpuHandle = reinterpret_cast<uint64_t>(module);
    return handle;
#else
    (void)bytecode;
    return {};
#endif
}

rhi::PipelineHandle VulkanDevice::createPipeline(const rhi::PipelineDesc& desc)
{
#ifdef VKT_HAS_VULKAN
    // [HACK: Full VkPipeline creation with VkPipelineLayout from TextureSlots/BufferSlots,
    //  VkPipelineCache, dynamic state for viewport/scissor — Task 16 完整实现]
    (void)desc;
#endif
    auto handle = AllocHandle<rhi::PipelineTag>();
    return handle;
}

rhi::GeometryHandle VulkanDevice::createGeometry(const rhi::GeometryDesc& desc)
{
    auto handle = AllocHandle<rhi::GeometryTag>();
    (void)desc;
    return handle;
}

rhi::SamplerHandle VulkanDevice::createSampler(const rhi::SamplerDesc& desc)
{
#ifdef VKT_HAS_VULKAN
    auto vkDevice = static_cast<VkDevice>(m_NativeDevice);
    if (!vkDevice) return {};

    auto ToVkFilter = [](rhi::FilterMode f) {
        return f == rhi::FilterMode::Linear ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    };
    auto ToVkSamplerMipmapMode = [](rhi::MipMode m) {
        return m == rhi::MipMode::Linear ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
    };
    auto ToVkAddressMode = [](rhi::WrapMode w) {
        switch (w) {
            case rhi::WrapMode::Repeat: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
            case rhi::WrapMode::ClampToEdge: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            case rhi::WrapMode::ClampToBorder: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            case rhi::WrapMode::MirroredRepeat: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            default: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        }
    };

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = ToVkFilter(desc.MagFilter);
    samplerInfo.minFilter = ToVkFilter(desc.MinFilter);
    samplerInfo.mipmapMode = ToVkSamplerMipmapMode(desc.Mip);
    samplerInfo.addressModeU = ToVkAddressMode(desc.WrapU);
    samplerInfo.addressModeV = ToVkAddressMode(desc.WrapV);
    samplerInfo.addressModeW = ToVkAddressMode(desc.WrapW);
    samplerInfo.maxAnisotropy = desc.MaxAnisotropy;
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

    VkSampler vkSampler;
    if (vkCreateSampler(vkDevice, &samplerInfo, nullptr, &vkSampler) != VK_SUCCESS)
        return {};

    auto handle = AllocHandle<rhi::SamplerTag>();
    GpuSlot* slot = GetSlot(handle.GetId());
    if (slot) slot->GpuHandle = reinterpret_cast<uint64_t>(vkSampler);
    return handle;
#else
    (void)desc;
    return {};
#endif
}

rhi::RenderPassHandle VulkanDevice::createRenderPass(const rhi::RenderPassDesc& desc)
{
#ifdef VKT_HAS_VULKAN
    // [HACK: Full VkRenderPass creation with attachments/subpasses — Task 18 实现]
    (void)desc;
#endif
    auto handle = AllocHandle<rhi::RenderPassTag>();
    return handle;
}

rhi::FramebufferHandle VulkanDevice::createFramebuffer(const rhi::FramebufferDesc& desc)
{
#ifdef VKT_HAS_VULKAN
    // [HACK: Full VkFramebuffer creation — Task 18 实现]
    (void)desc;
#endif
    auto handle = AllocHandle<rhi::FramebufferTag>();
    return handle;
}

void VulkanDevice::onResize(uint32_t width, uint32_t height)
{
    (void)width; (void)height;
    // [HACK: Swapchain recreation — Task 18 实现]
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

// ---- Handle Pool ----

template<typename Tag>
rhi::Handle<Tag> VulkanDevice::AllocHandle()
{
    uint32_t id = FindFreeSlot();
    GpuSlot& slot = m_Slots[id];
    slot.Alive = true;
    return rhi::Handle<Tag>(id, slot.Generation);
}

VulkanDevice::GpuSlot* VulkanDevice::GetSlot(uint32_t id)
{
    if (id >= m_Slots.size()) return nullptr;
    return &m_Slots[id];
}

uint32_t VulkanDevice::FindFreeSlot()
{
    if (!m_FreeIndices.empty())
    {
        uint32_t id = m_FreeIndices.back();
        m_FreeIndices.pop_back();
        m_Slots[id].Generation++;
        m_Slots[id].GpuHandle = 0;
        m_Slots[id].GpuHandle2 = 0;
        return id;
    }
    m_Slots.emplace_back();
    return static_cast<uint32_t>(m_Slots.size() - 1);
}

uint32_t VulkanDevice::FindMemoryType(uint32_t typeFilter, uint32_t properties)
{
#ifdef VKT_HAS_VULKAN
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(static_cast<VkPhysicalDevice>(m_PhysicalDevice), &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) &&
            (memProps.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }
#endif
    return 0;
}

} // namespace Vulkitten

