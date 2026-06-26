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
    if (desc.Usage == BufferUsage::Vertex)   usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    else if (desc.Usage == BufferUsage::Index)    usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    else if (desc.Usage == BufferUsage::Uniform)  usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    else if (desc.Usage == BufferUsage::Storage)  usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    bufferInfo.usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
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
// VKTextureResource (stub)
// ============================================================

VKTextureResource::VKTextureResource(VkDevice device, const TextureDesc& desc, const void* /*initialData*/)
    : m_Device(device)
    , m_Desc(desc)
{
}

VKTextureResource::~VKTextureResource() = default;

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
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
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

    // ---- Dynamic state ----
    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    // ---- Descriptor set layout (from pipeline slots) ----
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
        pcRange.stageFlags = (desc.ComputeShader.IsValid() ? VK_SHADER_STAGE_COMPUTE_BIT :
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
        return;
    }
    m_PipelineLayout = pipelineLayout;

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
// VKSamplerResource (stub)
// ============================================================

VKSamplerResource::VKSamplerResource(VkDevice device, const SamplerDesc&)
    : m_Device(device)
{
    // [HACK: VkSampler creation omitted for MVP]
}

VKSamplerResource::~VKSamplerResource()
{
    if (m_Sampler)
        vkDestroySampler(m_Device, m_Sampler, nullptr);
}

} // namespace rhi
