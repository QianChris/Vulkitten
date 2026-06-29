#include "VKDevice.hpp"
#include "VKCommandBuffer.hpp"
#include "VKResources.hpp"
#include "rhi/ResourceManager.hpp"

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <windows.h>
#include <vulkan/vulkan_win32.h>

#include <algorithm>
#include <stdexcept>
#include <cstring>
#include <cstdio>
#include <vector>

namespace rhi {

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
    do { VkResult r = (call); if (r != VK_SUCCESS) throw std::runtime_error(msg); } while(0)
#else
#define VK_CHECK_FATAL VK_CHECK
#endif

// ============================================================
// Construction
// ============================================================

VKDevice::VKDevice(ISurface* surface, ResourceManager& rm)
    : m_Surface(surface)
    , m_Resources(rm)
{
}

VKDevice::~VKDevice()
{
    Shutdown();
}

void VKDevice::Init()
{
    if (!m_Surface)
        throw std::runtime_error("VKDevice: no surface provided");

    // glfwInit is ref-counted; must be called from the DLL too because
    // GLFW is statically linked into both DLL and EXE — the DLL has its
    // own copy of GLFW global state that must be initialized before
    // calling glfwGetRequiredInstanceExtensions / glfwCreateWindowSurface.
    if (!glfwInit())
        throw std::runtime_error("VKDevice: glfwInit() failed");

    // === Create Instance ===
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "VulkittenRHI";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    std::vector<const char*> layers;
#ifndef NDEBUG
    layers.push_back("VK_LAYER_KHRONOS_validation");
#endif

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
        throw std::runtime_error("VKDevice: failed to create instance");
    m_Instance = instance;

    // === Pick Physical Device ===
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0)
        throw std::runtime_error("VKDevice: no Vulkan GPU found");
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    VkPhysicalDevice chosenDevice = devices[0];
    for (auto& d : devices) {
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(d, &props);
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) { chosenDevice = d; break; }
    }
    m_PhysicalDevice = chosenDevice;

    // === Find Queue Families ===
    uint32_t qfCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(chosenDevice, &qfCount, nullptr);
    std::vector<VkQueueFamilyProperties> qfProps(qfCount);
    vkGetPhysicalDeviceQueueFamilyProperties(chosenDevice, &qfCount, qfProps.data());

    GLFWwindow* window = static_cast<GLFWwindow*>(m_Surface->GetNativeHandle());
    VkSurfaceKHR tempSurface = VK_NULL_HANDLE;
    glfwCreateWindowSurface(instance, window, nullptr, &tempSurface);

    bool found = false;
    for (uint32_t i = 0; i < qfCount; ++i) {
        VkBool32 present = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(chosenDevice, i, tempSurface, &present);
        if (qfProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && present) {
            m_GraphicsQueueFamily = i;
            m_PresentQueueFamily = i;
            found = true; break;
        }
    }
    vkDestroySurfaceKHR(instance, tempSurface, nullptr);
    if (!found) throw std::runtime_error("VKDevice: no graphics+present queue");

    // === Create Logical Device ===
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = m_GraphicsQueueFamily;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &queuePriority;

    const char* deviceExts[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.enabledExtensionCount = 1;
    deviceInfo.ppEnabledExtensionNames = deviceExts;
    deviceInfo.pEnabledFeatures = &deviceFeatures;

    VkDevice device = VK_NULL_HANDLE;
    res = vkCreateDevice(chosenDevice, &deviceInfo, nullptr, &device);
    if (res != VK_SUCCESS) throw std::runtime_error("VKDevice: failed to create logical device");
    m_Device = device;

    vkGetDeviceQueue(device, m_GraphicsQueueFamily, 0, reinterpret_cast<VkQueue*>(&m_GraphicsQueue));
    vkGetDeviceQueue(device, m_PresentQueueFamily, 0, reinterpret_cast<VkQueue*>(&m_PresentQueue));
    m_FramesInFlight = 2;

    // === Swapchain ===
    m_Swapchain = std::make_unique<VKSwapchain>();
    if (!m_Swapchain->Create(m_Instance, m_PhysicalDevice, m_Device, m_Surface, m_FramesInFlight))
        throw std::runtime_error("VKDevice: failed to create swapchain");

    CreateCommandPools();
    CreateDescriptorPool();
    LoadDebugUtilsFunctions();
    m_Initialized = true;
}

void VKDevice::Shutdown()
{
    if (!m_Initialized) return;
    auto dev = static_cast<VkDevice>(m_Device);
    vkDeviceWaitIdle(dev);

    // ResourceManager::DestroyAll() handles RAII cleanup of all resources.
    // Called by Renderer (owner of ResourceManager), not here.

    // Clear descriptor set cache BEFORE destroying the descriptor pool
    m_DescriptorSetCache.clear();

    // Destroy query pools
    for (auto& [id, pool] : m_QueryPools)
        if (pool) vkDestroyQueryPool(dev, static_cast<VkQueryPool>(pool), nullptr);
    m_QueryPools.clear();

    DestroyDescriptorPool();
    DestroyCommandPools();

    m_Swapchain.reset();
    if (m_Device) { vkDestroyDevice(dev, nullptr); m_Device = nullptr; }
    if (m_Instance) { vkDestroyInstance(static_cast<VkInstance>(m_Instance), nullptr); m_Instance = nullptr; }
    m_Initialized = false;
}

// ============================================================
// Frame Lifecycle
// ============================================================

FrameContext VKDevice::BeginFrame()
{
    auto surfDesc = m_Surface->GetDesc();
    if (surfDesc.Width != m_Swapchain->GetWidth() || surfDesc.Height != m_Swapchain->GetHeight())
        OnResize(surfDesc.Width, surfDesc.Height);

    uint32_t imageIndex = 0;
    if (!m_Swapchain->AcquireNextImage(m_FrameIndex, &imageIndex))
    {
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
    if (!m_Swapchain->SubmitAndPresent(ctx.FrameIndex, ctx.SwapchainIndex, m_CurrentVkCmd, m_GraphicsQueue))
        OnResize(m_Swapchain->GetWidth(), m_Swapchain->GetHeight());

    m_CurrentVkCmd = nullptr;
    m_FrameIndex = (m_FrameIndex + 1) % m_FramesInFlight;
}

// ============================================================
// Command Buffer
// ============================================================

std::unique_ptr<ICommandBuffer> VKDevice::CreateCommandBuffer(FrameContext ctx, CommandBufferLevel)
{
    auto dev = static_cast<VkDevice>(m_Device);
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = static_cast<VkCommandPool>(GetCommandPool(ctx.FrameIndex));
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VK_CHECK(vkAllocateCommandBuffers(dev, &allocInfo, &cmd));
    m_CurrentVkCmd = cmd;
    return std::make_unique<VKCommandBuffer>(*this, cmd, ctx.FrameIndex);
}

// ============================================================
// Buffer
// ============================================================

BufferHandle VKDevice::CreateBuffer(const BufferDesc& desc, const void* initialData)
{
    auto dev = static_cast<VkDevice>(m_Device);

    // Create a temporary buffer to query memory requirements
    VkBufferCreateInfo tempInfo{};
    tempInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    tempInfo.size = desc.Size;
    tempInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    tempInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer tempBuf = VK_NULL_HANDLE;
    if (vkCreateBuffer(dev, &tempInfo, nullptr, &tempBuf) != VK_SUCCESS)
        return BufferHandle{};

    VkMemoryRequirements memReqs{};
    vkGetBufferMemoryRequirements(dev, tempBuf, &memReqs);
    vkDestroyBuffer(dev, tempBuf, nullptr);

    uint32_t memoryTypeIndex = static_cast<uint32_t>(FindMemoryType(
        memReqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

    uint32_t id = m_Resources.AllocateSlot();

    auto buffer = std::make_unique<VKBufferResource>(
        dev, desc, initialData, memoryTypeIndex);

    m_Resources.StoreBuffer(id, std::move(buffer));

    uint32_t gen = m_Resources.GetGeneration(id);
    return BufferHandle{id, gen};
}

// ============================================================
// Texture (stub)
// ============================================================

TextureHandle VKDevice::CreateTexture(const TextureDesc& desc, const void* initialData)
{
    uint32_t id = m_Resources.AllocateSlot();
    auto texture = std::make_unique<VKTextureResource>(
        static_cast<VkDevice>(m_Device),
        static_cast<VkPhysicalDevice>(m_PhysicalDevice),
        desc, initialData);
    m_Resources.StoreTexture(id, std::move(texture));
    uint32_t gen = m_Resources.GetGeneration(id);
    return TextureHandle{id, gen};
}

// ============================================================
// Shader
// ============================================================

ShaderHandle VKDevice::CreateShader(ShaderStage stage, const ShaderBytecode& bytecode)
{
    uint32_t id = m_Resources.AllocateSlot();

    auto shader = std::make_unique<VKShaderResource>(static_cast<VkDevice>(m_Device), stage, bytecode);

    if (!shader->IsValid())
    {
        m_Resources.FreeSlot(id);
        return ShaderHandle{};
    }

    m_Resources.StoreShader(id, std::move(shader));

    uint32_t gen = m_Resources.GetGeneration(id);
    return ShaderHandle{id, gen};
}

// ============================================================
// Pipeline
// ============================================================

PipelineHandle VKDevice::CreatePipeline(const PipelineDesc& desc)
{
    auto* vs = dynamic_cast<VKShaderResource*>(m_Resources.GetShader(desc.VertexShader));
    auto* fs = dynamic_cast<VKShaderResource*>(m_Resources.GetShader(desc.FragmentShader));
    auto* cs = dynamic_cast<VKShaderResource*>(m_Resources.GetShader(desc.ComputeShader));

    VkRenderPass rp = m_Swapchain ? static_cast<VkRenderPass>(m_Swapchain->GetRenderPass()) : VK_NULL_HANDLE;

    uint32_t id = m_Resources.AllocateSlot();

    auto pipeline = std::make_unique<VKPipelineResource>(
        static_cast<VkDevice>(m_Device), desc, vs, fs, cs, rp);

    if (pipeline->GetVkPipeline() == VK_NULL_HANDLE)
    {
        m_Resources.FreeSlot(id);
        return PipelineHandle{};
    }

    m_Resources.StorePipeline(id, std::move(pipeline));

    uint32_t gen = m_Resources.GetGeneration(id);
    return PipelineHandle{id, gen};
}

// ============================================================
// Geometry
// ============================================================

GeometryHandle VKDevice::CreateGeometry(const GeometryDesc& desc)
{
    uint32_t id = m_Resources.AllocateSlot();

    auto geometry = std::make_unique<VKGeometryResource>(desc);

    m_Resources.StoreGeometry(id, std::move(geometry));

    uint32_t gen = m_Resources.GetGeneration(id);
    return GeometryHandle{id, gen};
}

// ============================================================
// QueryPool
// ============================================================

QueryPoolHandle VKDevice::CreateQueryPool(const QueryPoolDesc& desc)
{
    auto dev = static_cast<VkDevice>(m_Device);

    VkQueryPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;

    switch (desc.Type)
    {
        case QueryType::Timestamp:         poolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP; break;
        case QueryType::Occlusion:         poolInfo.queryType = VK_QUERY_TYPE_OCCLUSION; break;
        case QueryType::PipelineStatistics: poolInfo.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS; break;
        default: poolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP; break;
    }
    poolInfo.queryCount = desc.Count;

    VkQueryPool pool = VK_NULL_HANDLE;
    if (vkCreateQueryPool(dev, &poolInfo, nullptr, &pool) != VK_SUCCESS)
        return QueryPoolHandle{};

    uint32_t id = m_Resources.AllocateSlot();
    m_QueryPools[id] = static_cast<void*>(pool);

    uint32_t gen = m_Resources.GetGeneration(id);
    return QueryPoolHandle{id, gen};
}

void* VKDevice::GetQueryPoolVK(uint32_t poolId) const
{
    auto it = m_QueryPools.find(poolId);
    return (it != m_QueryPools.end()) ? it->second : nullptr;
}

void VKDevice::LoadDebugUtilsFunctions()
{
    auto inst = static_cast<VkInstance>(m_Instance);
    m_pfnBeginDebugUtilsLabel = reinterpret_cast<void*>(
        vkGetInstanceProcAddr(inst, "vkCmdBeginDebugUtilsLabelEXT"));
    m_pfnEndDebugUtilsLabel = reinterpret_cast<void*>(
        vkGetInstanceProcAddr(inst, "vkCmdEndDebugUtilsLabelEXT"));
    m_pfnInsertDebugUtilsLabel = reinterpret_cast<void*>(
        vkGetInstanceProcAddr(inst, "vkCmdInsertDebugUtilsLabelEXT"));
}

// ============================================================
// Sampler
// ============================================================

SamplerHandle VKDevice::CreateSampler(const SamplerDesc& desc)
{
    uint32_t id = m_Resources.AllocateSlot();

    auto sampler = std::make_unique<VKSamplerResource>(static_cast<VkDevice>(m_Device), desc);

    m_Resources.StoreSampler(id, std::move(sampler));

    uint32_t gen = m_Resources.GetGeneration(id);
    return SamplerHandle{id, gen};
}

// ============================================================
// RenderPass
// ============================================================

RenderPassHandle VKDevice::CreateRenderPass(const RenderPassDesc& desc)
{
    uint32_t id = m_Resources.AllocateSlot();
    m_Resources.StoreRenderPassDesc(id, desc);

    // Also store the VkRenderPass pointer (from swapchain) for use by command buffer
    m_FramebufferHandles[id] = reinterpret_cast<uint64_t>(m_Swapchain->GetRenderPass());

    uint32_t gen = m_Resources.GetGeneration(id);
    return RenderPassHandle{id, gen};
}

// ============================================================
// Framebuffer
// ============================================================

FramebufferHandle VKDevice::CreateFramebuffer(const FramebufferDesc& desc)
{
    uint32_t id = m_Resources.AllocateSlot();
    m_Resources.StoreFramebufferDesc(id, desc);

    // Use sentinel to indicate swapchain framebuffers
    m_FramebufferHandles[id] = kSwapchainFramebufferSentinel;

    uint32_t gen = m_Resources.GetGeneration(id);
    return FramebufferHandle{id, gen};
}

// ============================================================
// Window Events
// ============================================================

void VKDevice::OnResize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0) return;
    auto dev = static_cast<VkDevice>(m_Device);
    vkDeviceWaitIdle(dev);
    m_Swapchain->Destroy();
    if (!m_Swapchain->Create(m_Instance, m_PhysicalDevice, m_Device, m_Surface, m_FramesInFlight))
        fprintf(stderr, "VKDevice: failed to recreate swapchain on resize\n");
}

void VKDevice::WaitIdle()
{
    if (m_Device) vkDeviceWaitIdle(static_cast<VkDevice>(m_Device));
}

// ============================================================
// Descriptor Pool
// ============================================================

void VKDevice::CreateDescriptorPool()
{
    auto dev = static_cast<VkDevice>(m_Device);

    VkDescriptorPoolSize poolSizes[] = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 16},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 8},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16},
    };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = 32;
    poolInfo.poolSizeCount = 3;
    poolInfo.pPoolSizes = poolSizes;

    VkDescriptorPool pool = VK_NULL_HANDLE;
    VK_CHECK(vkCreateDescriptorPool(dev, &poolInfo, nullptr, &pool));
    m_DescriptorPool = pool;
}

void VKDevice::DestroyDescriptorPool()
{
    auto dev = static_cast<VkDevice>(m_Device);
    if (m_DescriptorPool)
    {
        vkDestroyDescriptorPool(dev, static_cast<VkDescriptorPool>(m_DescriptorPool), nullptr);
        m_DescriptorPool = nullptr;
    }
}

void* VKDevice::AllocateDescriptorSet(void* descriptorSetLayout)
{
    auto dev = static_cast<VkDevice>(m_Device);
    auto dsl = static_cast<VkDescriptorSetLayout>(descriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = static_cast<VkDescriptorPool>(m_DescriptorPool);
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &dsl;

    VkDescriptorSet ds = VK_NULL_HANDLE;
    VkResult res = vkAllocateDescriptorSets(dev, &allocInfo, &ds);
    if (res != VK_SUCCESS) return nullptr;
    return ds;
}

uint32_t VKDevice::FindMemoryType(uint32_t typeFilter, uint32_t properties)
{
    VkPhysicalDeviceMemoryProperties memProps{};
    vkGetPhysicalDeviceMemoryProperties(static_cast<VkPhysicalDevice>(m_PhysicalDevice), &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i)
        if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    return 0;
}

void VKDevice::CreateCommandPools()
{
    auto dev = static_cast<VkDevice>(m_Device);
    m_CommandPools.resize(m_FramesInFlight);
    for (uint32_t i = 0; i < m_FramesInFlight; ++i) {
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
        if (pool) vkDestroyCommandPool(dev, static_cast<VkCommandPool>(pool), nullptr);
    m_CommandPools.clear();
}

void* VKDevice::GetCommandPool(uint32_t frameIndex) const
{
    return (frameIndex < m_CommandPools.size()) ? m_CommandPools[frameIndex] : nullptr;
}

void* VKDevice::GetSwapchainFramebuffer(uint32_t imageIndex) const
{
    return m_Swapchain ? m_Swapchain->GetFramebuffer(imageIndex) : nullptr;
}

// ============================================================
// Resource Query (delegates to ResourceManager)
// ============================================================

IBuffer*   VKDevice::GetBuffer(BufferHandle handle)     { return m_Resources.GetBuffer(handle); }
ITexture*  VKDevice::GetTexture(TextureHandle handle)    { return m_Resources.GetTexture(handle); }
IShader*   VKDevice::GetShader(ShaderHandle handle)      { return m_Resources.GetShader(handle); }
IPipeline* VKDevice::GetPipeline(PipelineHandle handle)  { return m_Resources.GetPipeline(handle); }
IGeometry* VKDevice::GetGeometry(GeometryHandle handle)  { return m_Resources.GetGeometry(handle); }
ISampler*  VKDevice::GetSampler(SamplerHandle handle)    { return m_Resources.GetSampler(handle); }

} // namespace rhi
