#include "VKResources.hpp"

#include <cstring>
#include <cstdio>

namespace rhi {

// ============================================================
// Helpers: RHI → Vulkan enum mapping
// ============================================================

static VkFormat ToVkFormat(Format f)
{
    switch (f) {
        case Format::R32_FLOAT:   return VK_FORMAT_R32_SFLOAT;
        case Format::RG32_FLOAT:  return VK_FORMAT_R32G32_SFLOAT;
        case Format::RGB32_FLOAT: return VK_FORMAT_R32G32B32_SFLOAT;
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
// VKBufferResource
// ============================================================

VKBufferResource::VKBufferResource(VkDevice device, const BufferDesc& desc,
                                   const void* initialData, uint32_t memoryTypeIndex)
    : m_Device(device)
    , m_Size(desc.Size)
    , m_Usage(desc.Usage)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = desc.Size;

    VkBufferUsageFlags usage = 0;
    if (HasUsage(desc.Usage, BufferUsage::Vertex))    usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (HasUsage(desc.Usage, BufferUsage::Index))     usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (HasUsage(desc.Usage, BufferUsage::Uniform))   usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (HasUsage(desc.Usage, BufferUsage::Storage))   usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    if (HasUsage(desc.Usage, BufferUsage::Indirect))  usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    if (HasUsage(desc.Usage, BufferUsage::TransferSrc)) usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    if (HasUsage(desc.Usage, BufferUsage::TransferDst)) usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (usage == 0) usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT; // fallback
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer buffer = VK_NULL_HANDLE;
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
    {
        fprintf(stderr, "VKBufferResource: failed to create buffer\n");
        return;
    }
    m_Buffer = buffer;

    VkMemoryRequirements memReqs{};
    vkGetBufferMemoryRequirements(device, buffer, &memReqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    VkDeviceMemory memory = VK_NULL_HANDLE;
    if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
    {
        fprintf(stderr, "VKBufferResource: failed to allocate memory\n");
        return;
    }
    m_Memory = memory;
    vkBindBufferMemory(device, buffer, memory, 0);

    if (initialData)
    {
        void* mapped = nullptr;
        vkMapMemory(device, memory, 0, desc.Size, 0, &mapped);
        memcpy(mapped, initialData, static_cast<size_t>(desc.Size));
        vkUnmapMemory(device, memory);
    }
}

VKBufferResource::~VKBufferResource()
{
    if (m_Buffer)
        vkDestroyBuffer(m_Device, m_Buffer, nullptr);
    if (m_Memory)
        vkFreeMemory(m_Device, m_Memory, nullptr);
}

void* VKBufferResource::Map(uint64_t offset, uint64_t size)
{
    if (!m_Memory) return nullptr;
    void* mapped = nullptr;
    vkMapMemory(m_Device, m_Memory, offset, size, 0, &mapped);
    return mapped;
}

void VKBufferResource::Unmap()
{
    if (m_Memory)
        vkUnmapMemory(m_Device, m_Memory);
}

void VKBufferResource::Flush(uint64_t offset, uint64_t size)
{
    VkMappedMemoryRange range{};
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.memory = m_Memory;
    range.offset = offset;
    range.size = size;
    vkFlushMappedMemoryRanges(m_Device, 1, &range);
}

// ============================================================
// VKTextureResource
// ============================================================

VKTextureResource::VKTextureResource(VkDevice device, VkPhysicalDevice physicalDevice,
                                     const TextureDesc& desc, const void* initialData)
    : m_Device(device)
    , m_Desc(desc)
{
    if (desc.Type != TextureType::Texture2D)
        return; // [HACK: only 2D textures supported for MVP]

    // Create VkImage
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM; // [HACK: format mapping incomplete]
    imageInfo.extent = {desc.Extent.Width, desc.Extent.Height, 1};
    imageInfo.mipLevels = desc.MipLevels;
    imageInfo.arrayLayers = desc.ArrayLayers;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;

    VkImageUsageFlags imgUsage = 0;
    if (HasUsage(desc.Usage, TextureUsage::Sampled))    imgUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if (HasUsage(desc.Usage, TextureUsage::ColorAttachment)) imgUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (HasUsage(desc.Usage, TextureUsage::DepthStencilAttachment)) imgUsage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if (HasUsage(desc.Usage, TextureUsage::Storage))    imgUsage |= VK_IMAGE_USAGE_STORAGE_BIT;
    if (HasUsage(desc.Usage, TextureUsage::TransferSrc)) imgUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (HasUsage(desc.Usage, TextureUsage::TransferDst)) imgUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (imgUsage == 0) imgUsage = VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.usage = imgUsage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(device, &imageInfo, nullptr, &m_Image) != VK_SUCCESS)
    {
        fprintf(stderr, "VKTextureResource: failed to create image\n");
        return;
    }

    // Allocate memory
    VkMemoryRequirements memReqs{};
    vkGetImageMemoryRequirements(device, m_Image, &memReqs);

    uint32_t memTypeIndex = FindMemoryType(physicalDevice, memReqs.memoryTypeBits,
                                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = memTypeIndex;

    if (vkAllocateMemory(device, &allocInfo, nullptr, &m_Memory) != VK_SUCCESS)
    {
        fprintf(stderr, "VKTextureResource: failed to allocate memory\n");
        return;
    }
    vkBindImageMemory(device, m_Image, m_Memory, 0);

    // Create image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_Image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = desc.MipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = desc.ArrayLayers;

    if (vkCreateImageView(device, &viewInfo, nullptr, &m_ImageView) != VK_SUCCESS)
        fprintf(stderr, "VKTextureResource: failed to create image view\n");

    (void)initialData; // upload not implemented yet
}

VKTextureResource::~VKTextureResource()
{
    if (m_ImageView) vkDestroyImageView(m_Device, m_ImageView, nullptr);
    if (m_Image)     vkDestroyImage(m_Device, m_Image, nullptr);
    if (m_Memory)    vkFreeMemory(m_Device, m_Memory, nullptr);
}

uint32_t VKTextureResource::FindMemoryType(VkPhysicalDevice physDev, uint32_t typeFilter,
                                            VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProps{};
    vkGetPhysicalDeviceMemoryProperties(physDev, &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i)
        if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    return 0;
}

TextureType VKTextureResource::GetType() const     { return m_Desc.Type; }
Format      VKTextureResource::GetFormat() const    { return m_Desc.Format; }
Extent3D    VKTextureResource::GetExtent() const    { return m_Desc.Extent; }
uint32_t    VKTextureResource::GetMipLevels() const { return m_Desc.MipLevels; }

// ============================================================
// VKShaderResource
// ============================================================

VKShaderResource::VKShaderResource(VkDevice device, ShaderStage stage, const ShaderBytecode& bytecode)
    : m_Device(device)
    , m_Stage(stage)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = bytecode.Size;
    createInfo.pCode = reinterpret_cast<const uint32_t*>(bytecode.Data);

    VkShaderModule module = VK_NULL_HANDLE;
    VkResult res = vkCreateShaderModule(device, &createInfo, nullptr, &module);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "VKShaderResource: failed to create shader module (%d)\n", res);
        return;
    }
    m_ShaderModule = module;
}

VKShaderResource::~VKShaderResource()
{
    if (m_ShaderModule)
        vkDestroyShaderModule(m_Device, m_ShaderModule, nullptr);
}

// ============================================================
// VKPipelineResource
// ============================================================

VKPipelineResource::VKPipelineResource(VkDevice device,
                                       const PipelineDesc& desc,
                                       VKShaderResource* vs,
                                       VKShaderResource* fs,
                                       VKShaderResource* cs,
                                       VkRenderPass renderPass)
    : m_Device(device)
    , m_IsCompute(desc.ComputeShader.IsValid())
    , m_VertexLayout(desc.VertexLayout)
    , m_BufferSlots(desc.BufferSlots)
    , m_TextureSlots(desc.TextureSlots)
    , m_PushConstantsSize(desc.PushConstantsSize)
{
    // ---- Build shader stages ----
    std::vector<VkPipelineShaderStageCreateInfo> stages;

    auto addStage = [&](VKShaderResource* shader) {
        if (!shader || !shader->IsValid()) return;
        VkPipelineShaderStageCreateInfo s{};
        s.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        s.stage = ToVkShaderStage(shader->GetStage());
        s.module = shader->GetVkShaderModule();
        s.pName = "main";
        stages.push_back(s);
    };

    addStage(vs);
    addStage(fs);
    addStage(cs);

    if (stages.empty())
    {
        fprintf(stderr, "VKPipelineResource: no valid shader stages\n");
        return;
    }

    // ---- Descriptor set layout (from pipeline slots) ----
    // For compute pipelines with no explicit slots, create default storage
    // buffer bindings for slots 0-3. This covers the common case where
    // shaders declare bindings without explicit pipeline slot declarations.
    // TODO: replace with proper SPIR-V reflection.
    std::vector<BufferSlot> effectiveBufferSlots = desc.BufferSlots;
    std::vector<TextureSlot> effectiveTextureSlots = desc.TextureSlots;
    if (m_IsCompute && effectiveBufferSlots.empty() && effectiveTextureSlots.empty())
    {
        for (uint32_t s = 0; s < 4; ++s)
        {
            BufferSlot slot;
            slot.Slot = s;
            slot.SlotType = BufferSlot::Type::Storage;
            slot.Stages = ShaderStage::Compute;
            effectiveBufferSlots.push_back(slot);
        }
    }

    std::vector<VkDescriptorSetLayoutBinding> dslBindings;
    for (auto& bs : effectiveBufferSlots)
    {
        VkDescriptorSetLayoutBinding lb{};
        lb.binding = bs.Slot;
        lb.descriptorType = (bs.SlotType == BufferSlot::Type::Uniform) ?
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        lb.descriptorCount = 1;
        lb.stageFlags = ToVkShaderStage(bs.Stages);
        dslBindings.push_back(lb);
    }
    for (auto& ts : effectiveTextureSlots)
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
    if (vkCreateDescriptorSetLayout(device, &dslInfo, nullptr, &dsl) != VK_SUCCESS)
    {
        fprintf(stderr, "VKPipelineResource: failed to create descriptor set layout\n");
        return;
    }
    m_DescriptorSetLayout = dsl;

    // ---- Pipeline layout ----
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &dsl;

    if (desc.PushConstantsSize > 0)
    {
        VkPushConstantRange pcRange{};
        pcRange.stageFlags = (m_IsCompute ? VK_SHADER_STAGE_COMPUTE_BIT :
                              VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        pcRange.offset = 0;
        pcRange.size = desc.PushConstantsSize;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pcRange;
    }

    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
    {
        fprintf(stderr, "VKPipelineResource: failed to create pipeline layout\n");
        vkDestroyDescriptorSetLayout(device, dsl, nullptr);
        return;
    }
    m_PipelineLayout = pipelineLayout;

    // ============================================================
    // Compute pipeline
    // ============================================================
    if (m_IsCompute)
    {
        VkComputePipelineCreateInfo computeInfo{};
        computeInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        computeInfo.stage = stages[0];
        computeInfo.layout = pipelineLayout;

        VkPipeline pipeline = VK_NULL_HANDLE;
        VkResult res = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &computeInfo, nullptr, &pipeline);
        if (res != VK_SUCCESS)
        {
            fprintf(stderr, "VKPipelineResource: vkCreateComputePipelines failed (%d)\n", res);
            vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
            vkDestroyDescriptorSetLayout(device, dsl, nullptr);
            m_PipelineLayout = VK_NULL_HANDLE;
            m_DescriptorSetLayout = VK_NULL_HANDLE;
            return;
        }
        m_Pipeline = pipeline;
        return;
    }

    // ============================================================
    // Graphics pipeline
    // ============================================================
    // ---- Vertex input state ----
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;

    for (auto& attr : desc.VertexLayout)
    {
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

    // ---- Input assembly ----
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // ---- Viewport state ----
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // ---- Rasterization ----
    VkPolygonMode vkPolyMode = VK_POLYGON_MODE_FILL;
    switch (desc.Raster.Poly)
    {
        case RasterState::PolygonMode::Fill:  vkPolyMode = VK_POLYGON_MODE_FILL;  break;
        case RasterState::PolygonMode::Line:  vkPolyMode = VK_POLYGON_MODE_LINE;  break;
        case RasterState::PolygonMode::Point: vkPolyMode = VK_POLYGON_MODE_POINT; break;
    }

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = desc.Raster.DepthClampEnable ? VK_TRUE : VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = vkPolyMode;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = ToVkCullMode(desc.Raster.Cull);
    rasterizer.frontFace = (desc.Raster.Front == RasterState::FrontFace::CounterClockwise) ?
        VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;

    // ---- Multisampling ----
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // ---- Depth/stencil ----
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = desc.DepthStencil.DepthTestEnable ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = desc.DepthStencil.DepthWriteEnable ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = ToVkCompareOp(desc.DepthStencil.DepthCompareOp);

    // ---- Color blend ----
    auto ToVkBlendFactor = [](BlendState::BlendFactor f) -> VkBlendFactor {
        switch (f) {
            case BlendState::BlendFactor::Zero:             return VK_BLEND_FACTOR_ZERO;
            case BlendState::BlendFactor::One:              return VK_BLEND_FACTOR_ONE;
            case BlendState::BlendFactor::SrcColor:         return VK_BLEND_FACTOR_SRC_COLOR;
            case BlendState::BlendFactor::OneMinusSrcColor: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
            case BlendState::BlendFactor::SrcAlpha:         return VK_BLEND_FACTOR_SRC_ALPHA;
            case BlendState::BlendFactor::OneMinusSrcAlpha: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            case BlendState::BlendFactor::DstAlpha:         return VK_BLEND_FACTOR_DST_ALPHA;
            case BlendState::BlendFactor::DstColor:         return VK_BLEND_FACTOR_DST_COLOR;
            default: return VK_BLEND_FACTOR_ONE;
        }
    };
    auto ToVkBlendOp = [](BlendState::BlendOp op) -> VkBlendOp {
        switch (op) {
            case BlendState::BlendOp::Add:             return VK_BLEND_OP_ADD;
            case BlendState::BlendOp::Subtract:        return VK_BLEND_OP_SUBTRACT;
            case BlendState::BlendOp::ReverseSubtract: return VK_BLEND_OP_REVERSE_SUBTRACT;
            case BlendState::BlendOp::Min:             return VK_BLEND_OP_MIN;
            case BlendState::BlendOp::Max:             return VK_BLEND_OP_MAX;
            default: return VK_BLEND_OP_ADD;
        }
    };

    std::vector<VkPipelineColorBlendAttachmentState> blendAttachments;
    for (auto& b : desc.Blends)
    {
        VkPipelineColorBlendAttachmentState ba{};
        ba.blendEnable = b.Enable ? VK_TRUE : VK_FALSE;
        ba.srcColorBlendFactor = ToVkBlendFactor(b.SrcColorFactor);
        ba.dstColorBlendFactor = ToVkBlendFactor(b.DstColorFactor);
        ba.colorBlendOp = ToVkBlendOp(b.ColorOp);
        ba.srcAlphaBlendFactor = ToVkBlendFactor(b.SrcAlphaFactor);
        ba.dstAlphaBlendFactor = ToVkBlendFactor(b.DstAlphaFactor);
        ba.alphaBlendOp = ToVkBlendOp(b.AlphaOp);
        ba.colorWriteMask = (b.WriteR ? VK_COLOR_COMPONENT_R_BIT : 0) |
                            (b.WriteG ? VK_COLOR_COMPONENT_G_BIT : 0) |
                            (b.WriteB ? VK_COLOR_COMPONENT_B_BIT : 0) |
                            (b.WriteA ? VK_COLOR_COMPONENT_A_BIT : 0);
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

    // ---- Dynamic state ----
    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    // ---- Graphics pipeline ----
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
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    VkPipeline pipeline = VK_NULL_HANDLE;
    VkResult pipeRes = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
    if (pipeRes != VK_SUCCESS)
    {
        fprintf(stderr, "VKPipelineResource: vkCreateGraphicsPipelines failed (%d)\n", pipeRes);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyDescriptorSetLayout(device, dsl, nullptr);
        m_PipelineLayout = VK_NULL_HANDLE;
        m_DescriptorSetLayout = VK_NULL_HANDLE;
        return;
    }
    m_Pipeline = pipeline;
}

VKPipelineResource::~VKPipelineResource()
{
    if (m_Pipeline)
        vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
    if (m_PipelineLayout)
        vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
    if (m_DescriptorSetLayout)
        vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
}

// ============================================================
// VKGeometryResource
// ============================================================

VKGeometryResource::VKGeometryResource(const GeometryDesc& desc)
    : m_Desc(desc)
{
}

// ============================================================
// VKSamplerResource
// ============================================================

static VkFilter ToVkFilter(FilterMode m) { return m == FilterMode::Nearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR; }
static VkSamplerMipmapMode ToVkMipMode(MipMode m) { return m == MipMode::Nearest ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR; }
static VkSamplerAddressMode ToVkWrap(WrapMode w) {
    switch (w) {
        case WrapMode::Repeat:         return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case WrapMode::ClampToEdge:    return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case WrapMode::ClampToBorder:  return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        case WrapMode::MirroredRepeat: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        default: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
}

VKSamplerResource::VKSamplerResource(VkDevice device, const SamplerDesc& desc)
    : m_Device(device)
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = ToVkFilter(desc.MagFilter);
    samplerInfo.minFilter = ToVkFilter(desc.MinFilter);
    samplerInfo.mipmapMode = ToVkMipMode(desc.Mip);
    samplerInfo.addressModeU = ToVkWrap(desc.WrapU);
    samplerInfo.addressModeV = ToVkWrap(desc.WrapV);
    samplerInfo.addressModeW = ToVkWrap(desc.WrapW);
    samplerInfo.anisotropyEnable = desc.MaxAnisotropy > 1.0f ? VK_TRUE : VK_FALSE;
    samplerInfo.maxAnisotropy = desc.MaxAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

    vkCreateSampler(device, &samplerInfo, nullptr, &m_Sampler);
}

VKSamplerResource::~VKSamplerResource()
{
    if (m_Sampler)
        vkDestroySampler(m_Device, m_Sampler, nullptr);
}

} // namespace rhi
