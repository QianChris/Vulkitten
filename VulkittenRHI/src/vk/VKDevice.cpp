#include "VKDevice.hpp"
#include "VKCommandBuffer.hpp"

#include "rhi/IBuffer.hpp"
#include "rhi/ITexture.hpp"
#include "rhi/IShader.hpp"
#include "rhi/IPipeline.hpp"
#include "rhi/IGeometry.hpp"
#include "rhi/ISampler.hpp"

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
// Internal query interface implementations
// ============================================================

class VKBuffer final : public IBuffer
{
public:
    VKBuffer(const VKBufferMeta& meta) : m_Meta(meta) {}
    uint64_t GetSize() const override { return m_Meta.Size; }
    void* Map(uint64_t /*offset*/, uint64_t /*size*/) override { return nullptr; }
    void Unmap() override {}
    void Flush(uint64_t, uint64_t) override {}
private:
    const VKBufferMeta& m_Meta;
};

class VKTexture final : public ITexture
{
public:
    TextureType GetType() const override { return TextureType::Texture2D; }
    Format GetFormat() const override { return Format::Unknown; }
    Extent3D GetExtent() const override { return {}; }
    uint32_t GetMipLevels() const override { return 1; }
};

class VKShaderImpl final : public IShader
{
public:
    VKShaderImpl(const VKShaderMeta& meta) : m_Meta(meta) {}
    ShaderStage GetStage() const override { return m_Meta.Stage; }
    const char* GetEntryPoint() const override { return "main"; }
private:
    const VKShaderMeta& m_Meta;
};

class VKPipelineImpl final : public IPipeline
{
public:
    bool IsCompute() const override { return false; }
};

class VKGeometryImpl final : public IGeometry
{
public:
    VKGeometryImpl(const VKGeometryMeta& meta) : m_Meta(meta) {}
    uint32_t GetVertexCount() const override { return m_Meta.Desc.VertexCount; }
    uint32_t GetIndexCount() const override { return m_Meta.Desc.IndexCount; }
private:
    const VKGeometryMeta& m_Meta;
};

class VKSamplerImpl final : public ISampler {};

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
// Helpers: RHI → Vulkan enum mapping
// ============================================================

static VkFormat ToVkFormat(Format f)
{
    switch (f) {
        case Format::R32_FLOAT:  return VK_FORMAT_R32_SFLOAT;
        case Format::RG32_FLOAT: return VK_FORMAT_R32G32_SFLOAT;
        case Format::RGB32_FLOAT:return VK_FORMAT_R32G32B32_SFLOAT;
        case Format::RGBA32_FLOAT:return VK_FORMAT_R32G32B32A32_SFLOAT;
        case Format::BGRA8_UNORM: return VK_FORMAT_B8G8R8A8_UNORM;
        default: return VK_FORMAT_UNDEFINED;
    }
}

static VkShaderStageFlagBits ToVkShaderStage(ShaderStage stage)
{
    switch (stage) {
        case ShaderStage::Vertex:   return VK_SHADER_STAGE_VERTEX_BIT;
        case ShaderStage::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
        case ShaderStage::Compute:  return VK_SHADER_STAGE_COMPUTE_BIT;
        default: return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
    }
}

static VkCullModeFlags ToVkCullMode(RasterState::CullMode m)
{
    switch (m) {
        case RasterState::CullMode::None:  return VK_CULL_MODE_NONE;
        case RasterState::CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
        case RasterState::CullMode::Back:  return VK_CULL_MODE_BACK_BIT;
        default: return VK_CULL_MODE_BACK_BIT;
    }
}

static VkCompareOp ToVkCompareOp(DepthStencilState::CompareOp op)
{
    switch (op) {
        case DepthStencilState::CompareOp::Never:    return VK_COMPARE_OP_NEVER;
        case DepthStencilState::CompareOp::Less:     return VK_COMPARE_OP_LESS;
        case DepthStencilState::CompareOp::Equal:    return VK_COMPARE_OP_EQUAL;
        case DepthStencilState::CompareOp::LessEqual:return VK_COMPARE_OP_LESS_OR_EQUAL;
        case DepthStencilState::CompareOp::Greater:  return VK_COMPARE_OP_GREATER;
        case DepthStencilState::CompareOp::NotEqual: return VK_COMPARE_OP_NOT_EQUAL;
        case DepthStencilState::CompareOp::GreaterEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case DepthStencilState::CompareOp::Always:   return VK_COMPARE_OP_ALWAYS;
        default: return VK_COMPARE_OP_LESS;
    }
}

// ============================================================
// Construction
// ============================================================

VKDevice::VKDevice(ISurface* surface)
    : m_Surface(surface)
{
    m_Slots.push_back({0, 0, 1, false});
}

VKDevice::~VKDevice()
{
    Shutdown();
}

void VKDevice::Init()
{
    if (!m_Surface)
        throw std::runtime_error("VKDevice: no surface provided");

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
    m_Initialized = true;
}

void VKDevice::Shutdown()
{
    if (!m_Initialized) return;
    auto dev = static_cast<VkDevice>(m_Device);
    vkDeviceWaitIdle(dev);

    DestroyDescriptorPool();
    DestroyCommandPools();

    // Clean up resources
    for (auto& [id, meta] : m_PipelineMetas) {
        if (meta.Pipeline) vkDestroyPipeline(dev, static_cast<VkPipeline>(meta.Pipeline), nullptr);
        if (meta.PipelineLayout) vkDestroyPipelineLayout(dev, static_cast<VkPipelineLayout>(meta.PipelineLayout), nullptr);
        if (meta.DescriptorSetLayout) vkDestroyDescriptorSetLayout(dev, static_cast<VkDescriptorSetLayout>(meta.DescriptorSetLayout), nullptr);
    }
    for (auto& [id, meta] : m_ShaderMetas) {
        if (meta.ShaderModule) vkDestroyShaderModule(dev, static_cast<VkShaderModule>(meta.ShaderModule), nullptr);
    }
    for (auto& [id, meta] : m_BufferMetas) {
        if (meta.Buffer) vkDestroyBuffer(dev, static_cast<VkBuffer>(meta.Buffer), nullptr);
        if (meta.Memory) vkFreeMemory(dev, static_cast<VkDeviceMemory>(meta.Memory), nullptr);
    }
    m_PipelineMetas.clear();
    m_ShaderMetas.clear();
    m_BufferMetas.clear();
    m_GeometryMetas.clear();

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

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = desc.Size;

    VkBufferUsageFlags usage = 0;
    if (desc.Usage == BufferUsage::Vertex)   usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    else if (desc.Usage == BufferUsage::Index)    usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    else if (desc.Usage == BufferUsage::Uniform)  usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    else if (desc.Usage == BufferUsage::Storage)  usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    bufferInfo.usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer buffer = VK_NULL_HANDLE;
    VK_CHECK(vkCreateBuffer(dev, &bufferInfo, nullptr, &buffer));

    // Allocate memory (host-visible for MVP simplicity)
    VkMemoryRequirements memReqs{};
    vkGetBufferMemoryRequirements(dev, buffer, &memReqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memReqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkDeviceMemory memory = VK_NULL_HANDLE;
    VK_CHECK(vkAllocateMemory(dev, &allocInfo, nullptr, &memory));
    vkBindBufferMemory(dev, buffer, memory, 0);

    // Upload initial data
    if (initialData)
    {
        void* mapped = nullptr;
        vkMapMemory(dev, memory, 0, desc.Size, 0, &mapped);
        memcpy(mapped, initialData, static_cast<size_t>(desc.Size));
        vkUnmapMemory(dev, memory);
    }

    uint32_t id = FindFreeSlot();
    m_Slots[id].Alive = true;
    m_Slots[id].GpuHandle = reinterpret_cast<uint64_t>(buffer);
    m_Slots[id].GpuHandle2 = reinterpret_cast<uint64_t>(memory);

    VKBufferMeta meta;
    meta.Buffer = buffer;
    meta.Memory = memory;
    meta.Size = desc.Size;
    meta.Usage = desc.Usage;
    m_BufferMetas[id] = meta;

    return BufferHandle{id, m_Slots[id].Generation};
}

// ============================================================
// Texture (stub)
// ============================================================

TextureHandle VKDevice::CreateTexture(const TextureDesc&, const void*) { return TextureHandle{}; }

// ============================================================
// Shader
// ============================================================

ShaderHandle VKDevice::CreateShader(ShaderStage stage, const ShaderBytecode& bytecode)
{
    auto dev = static_cast<VkDevice>(m_Device);

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = bytecode.Size;
    createInfo.pCode = reinterpret_cast<const uint32_t*>(bytecode.Data);

    VkShaderModule module = VK_NULL_HANDLE;
    VkResult res = vkCreateShaderModule(dev, &createInfo, nullptr, &module);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "VKDevice::CreateShader: failed to create shader module (%d)\n", res);
        return ShaderHandle{};
    }

    uint32_t id = FindFreeSlot();
    m_Slots[id].Alive = true;
    m_Slots[id].GpuHandle = reinterpret_cast<uint64_t>(module);

    VKShaderMeta meta;
    meta.ShaderModule = module;
    meta.Stage = stage;
    m_ShaderMetas[id] = meta;

    return ShaderHandle{id, m_Slots[id].Generation};
}

// ============================================================
// Pipeline
// ============================================================

PipelineHandle VKDevice::CreatePipeline(const PipelineDesc& desc)
{
    auto dev = static_cast<VkDevice>(m_Device);

    // Build shader stages
    std::vector<VkPipelineShaderStageCreateInfo> stages;

    auto addStage = [&](ShaderHandle handle) {
        if (!handle.IsValid()) return;
        auto it = m_ShaderMetas.find(handle.GetId());
        if (it == m_ShaderMetas.end()) return;
        VkPipelineShaderStageCreateInfo s{};
        s.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        s.stage = ToVkShaderStage(it->second.Stage);
        s.module = static_cast<VkShaderModule>(it->second.ShaderModule);
        s.pName = "main";
        stages.push_back(s);
    };

    addStage(desc.VertexShader);
    addStage(desc.FragmentShader);

    if (stages.empty())
    {
        fprintf(stderr, "VKDevice::CreatePipeline: no valid shader stages\n");
        return PipelineHandle{};
    }

    // Vertex input state
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;

    for (auto& attr : desc.VertexLayout)
    {
        // Check if binding already exists
        bool bindingExists = false;
        for (auto& b : bindings) {
            if (b.binding == attr.BufferSlot) { bindingExists = true; break; }
        }
        if (!bindingExists) {
            VkVertexInputBindingDescription b{};
            b.binding = attr.BufferSlot;
            b.stride = attr.Stride;
            b.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            bindings.push_back(b);
        }

        VkVertexInputAttributeDescription a{};
        a.location = attr.Location;
        a.binding = attr.BufferSlot;
        a.format = ToVkFormat(attr.Format);
        a.offset = attr.Offset;
        attributes.push_back(a);
    }

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
    vertexInput.pVertexBindingDescriptions = bindings.data();
    vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
    vertexInput.pVertexAttributeDescriptions = attributes.data();

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport state
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // Rasterization
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = ToVkCullMode(desc.Raster.Cull);
    rasterizer.frontFace = (desc.Raster.Front == RasterState::FrontFace::CounterClockwise) ?
        VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Depth/stencil
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = desc.DepthStencil.DepthTestEnable ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = desc.DepthStencil.DepthWriteEnable ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = ToVkCompareOp(desc.DepthStencil.DepthCompareOp);

    // Color blend
    std::vector<VkPipelineColorBlendAttachmentState> blendAttachments;
    for (auto& b : desc.Blends)
    {
        VkPipelineColorBlendAttachmentState ba{};
        ba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        blendAttachments.push_back(ba);
    }
    if (blendAttachments.empty())
    {
        VkPipelineColorBlendAttachmentState ba{};
        ba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        blendAttachments.push_back(ba);
    }

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = static_cast<uint32_t>(blendAttachments.size());
    colorBlending.pAttachments = blendAttachments.data();

    // Dynamic state
    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    // Descriptor set layout (from pipeline slots)
    std::vector<VkDescriptorSetLayoutBinding> dslBindings;
    for (auto& bs : desc.BufferSlots)
    {
        VkDescriptorSetLayoutBinding lb{};
        lb.binding = bs.Slot;
        lb.descriptorType = (bs.SlotType == BufferSlot::Type::Uniform) ?
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        lb.descriptorCount = 1;
        lb.stageFlags = ToVkShaderStage(bs.Stages);
        dslBindings.push_back(lb);
    }
    for (auto& ts : desc.TextureSlots)
    {
        VkDescriptorSetLayoutBinding lb{};
        lb.binding = ts.Slot;
        lb.descriptorType = (ts.SlotType == TextureSlot::Type::Sampled) ?
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        lb.descriptorCount = 1;
        lb.stageFlags = ToVkShaderStage(ts.Stages);
        dslBindings.push_back(lb);
    }

    VkDescriptorSetLayoutCreateInfo dslInfo{};
    dslInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dslInfo.bindingCount = static_cast<uint32_t>(dslBindings.size());
    dslInfo.pBindings = dslBindings.data();

    VkDescriptorSetLayout dsl = VK_NULL_HANDLE;
    VK_CHECK(vkCreateDescriptorSetLayout(dev, &dslInfo, nullptr, &dsl));

    // Pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &dsl;

    if (desc.PushConstantsSize > 0)
    {
        VkPushConstantRange pcRange{};
        pcRange.stageFlags = (desc.ComputeShader.IsValid() ? VK_SHADER_STAGE_COMPUTE_BIT :
                              VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        pcRange.offset = 0;
        pcRange.size = desc.PushConstantsSize;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pcRange;
    }

    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VK_CHECK(vkCreatePipelineLayout(dev, &pipelineLayoutInfo, nullptr, &pipelineLayout));

    // Graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(stages.size());
    pipelineInfo.pStages = stages.data();
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = static_cast<VkRenderPass>(m_Swapchain->GetRenderPass());
    pipelineInfo.subpass = 0;

    VkPipeline pipeline = VK_NULL_HANDLE;
    VkResult pipeRes = vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
    if (pipeRes != VK_SUCCESS)
    {
        fprintf(stderr, "VKDevice::CreatePipeline: vkCreateGraphicsPipelines failed (%d)\n", pipeRes);
        vkDestroyPipelineLayout(dev, pipelineLayout, nullptr);
        vkDestroyDescriptorSetLayout(dev, dsl, nullptr);
        return PipelineHandle{};
    }

    uint32_t id = FindFreeSlot();
    m_Slots[id].Alive = true;
    m_Slots[id].GpuHandle = reinterpret_cast<uint64_t>(pipeline);

    VKPipelineMeta meta;
    meta.Pipeline = pipeline;
    meta.PipelineLayout = pipelineLayout;
    meta.DescriptorSetLayout = dsl;
    meta.VertexLayout = desc.VertexLayout;
    meta.BufferSlots = desc.BufferSlots;
    meta.TextureSlots = desc.TextureSlots;
    m_PipelineMetas[id] = meta;

    return PipelineHandle{id, m_Slots[id].Generation};
}

// ============================================================
// Geometry
// ============================================================

GeometryHandle VKDevice::CreateGeometry(const GeometryDesc& desc)
{
    uint32_t id = FindFreeSlot();
    m_Slots[id].Alive = true;
    m_Slots[id].GpuHandle = 0;

    VKGeometryMeta meta;
    meta.Desc = desc;
    m_GeometryMetas[id] = meta;

    return GeometryHandle{id, m_Slots[id].Generation};
}

// ============================================================
// Sampler (stub)
// ============================================================

SamplerHandle VKDevice::CreateSampler(const SamplerDesc&) { return SamplerHandle{}; }

// ============================================================
// RenderPass
// ============================================================

RenderPassHandle VKDevice::CreateRenderPass(const RenderPassDesc&)
{
    m_DefaultRenderPassSlotId = FindFreeSlot();
    m_Slots[m_DefaultRenderPassSlotId].Alive = true;
    m_Slots[m_DefaultRenderPassSlotId].GpuHandle = reinterpret_cast<uint64_t>(m_Swapchain->GetRenderPass());
    return RenderPassHandle{m_DefaultRenderPassSlotId, m_Slots[m_DefaultRenderPassSlotId].Generation};
}

// ============================================================
// Framebuffer
// ============================================================

FramebufferHandle VKDevice::CreateFramebuffer(const FramebufferDesc&)
{
    uint32_t id = FindFreeSlot();
    m_Slots[id].Alive = true;
    m_Slots[id].GpuHandle = kSwapchainFramebufferSentinel;
    return FramebufferHandle{id, m_Slots[id].Generation};
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
    if (m_DefaultRenderPassSlotId > 0 && m_DefaultRenderPassSlotId < m_Slots.size())
        m_Slots[m_DefaultRenderPassSlotId].GpuHandle = reinterpret_cast<uint64_t>(m_Swapchain->GetRenderPass());
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

// ============================================================
// Metadata Access
// ============================================================

VKDevice::GpuSlot* VKDevice::GetSlot(uint32_t id)
{
    if (id == 0 || id >= m_Slots.size()) return nullptr;
    return &m_Slots[id];
}

const VKPipelineMeta* VKDevice::GetPipelineMeta(uint32_t id) const
{
    auto it = m_PipelineMetas.find(id);
    return (it != m_PipelineMetas.end()) ? &it->second : nullptr;
}

const VKBufferMeta* VKDevice::GetBufferMeta(uint32_t id) const
{
    auto it = m_BufferMetas.find(id);
    return (it != m_BufferMetas.end()) ? &it->second : nullptr;
}

const VKGeometryMeta* VKDevice::GetGeometryMeta(uint32_t id) const
{
    auto it = m_GeometryMetas.find(id);
    return (it != m_GeometryMetas.end()) ? &it->second : nullptr;
}

// ============================================================
// Internal
// ============================================================

uint32_t VKDevice::FindFreeSlot()
{
    if (!m_FreeIndices.empty()) {
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
// Resource Query
// ============================================================

IBuffer* VKDevice::GetBuffer(BufferHandle handle)
{
    if (!handle.IsValid()) return nullptr;
    auto it = m_BufferQueries.find(handle.GetId());
    if (it != m_BufferQueries.end()) return it->second.get();
    auto metaIt = m_BufferMetas.find(handle.GetId());
    if (metaIt == m_BufferMetas.end()) return nullptr;
    auto ptr = std::make_unique<VKBuffer>(metaIt->second);
    IBuffer* raw = ptr.get();
    m_BufferQueries[handle.GetId()] = std::move(ptr);
    return raw;
}

ITexture* VKDevice::GetTexture(TextureHandle handle)
{
    if (!handle.IsValid()) return nullptr;
    auto it = m_TextureQueries.find(handle.GetId());
    if (it != m_TextureQueries.end()) return it->second.get();
    auto ptr = std::make_unique<VKTexture>();
    ITexture* raw = ptr.get();
    m_TextureQueries[handle.GetId()] = std::move(ptr);
    return raw;
}

IShader* VKDevice::GetShader(ShaderHandle handle)
{
    if (!handle.IsValid()) return nullptr;
    auto it = m_ShaderQueries.find(handle.GetId());
    if (it != m_ShaderQueries.end()) return it->second.get();
    auto metaIt = m_ShaderMetas.find(handle.GetId());
    if (metaIt == m_ShaderMetas.end()) return nullptr;
    auto ptr = std::make_unique<VKShaderImpl>(metaIt->second);
    IShader* raw = ptr.get();
    m_ShaderQueries[handle.GetId()] = std::move(ptr);
    return raw;
}

IPipeline* VKDevice::GetPipeline(PipelineHandle handle)
{
    if (!handle.IsValid()) return nullptr;
    auto it = m_PipelineQueries.find(handle.GetId());
    if (it != m_PipelineQueries.end()) return it->second.get();
    auto ptr = std::make_unique<VKPipelineImpl>();
    IPipeline* raw = ptr.get();
    m_PipelineQueries[handle.GetId()] = std::move(ptr);
    return raw;
}

IGeometry* VKDevice::GetGeometry(GeometryHandle handle)
{
    if (!handle.IsValid()) return nullptr;
    auto it = m_GeometryQueries.find(handle.GetId());
    if (it != m_GeometryQueries.end()) return it->second.get();
    auto metaIt = m_GeometryMetas.find(handle.GetId());
    if (metaIt == m_GeometryMetas.end()) return nullptr;
    auto ptr = std::make_unique<VKGeometryImpl>(metaIt->second);
    IGeometry* raw = ptr.get();
    m_GeometryQueries[handle.GetId()] = std::move(ptr);
    return raw;
}

ISampler* VKDevice::GetSampler(SamplerHandle handle)
{
    if (!handle.IsValid()) return nullptr;
    auto it = m_SamplerQueries.find(handle.GetId());
    if (it != m_SamplerQueries.end()) return it->second.get();
    auto ptr = std::make_unique<VKSamplerImpl>();
    ISampler* raw = ptr.get();
    m_SamplerQueries[handle.GetId()] = std::move(ptr);
    return raw;
}

} // namespace rhi
