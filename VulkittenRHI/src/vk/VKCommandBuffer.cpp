#include "VKCommandBuffer.hpp"
#include "VKDevice.hpp"
#include "VKResources.hpp"
#include "VKSwapchain.hpp"
#include "rhi/ResourceManager.hpp"

#include <vulkan/vulkan.h>

#include <cstring>
#include <cstdio>
#include <algorithm>

namespace rhi {

// ============================================================
// RHI → VK enum translation (for barriers)
// ============================================================

static VkPipelineStageFlags ToVkPipelineStage(PipelineStage stage)
{
    VkPipelineStageFlags result = 0;
    if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(PipelineStage::VertexShader))
        result |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
    if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(PipelineStage::FragmentShader))
        result |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(PipelineStage::ComputeShader))
        result |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(PipelineStage::ColorAttachmentOutput))
        result |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(PipelineStage::Transfer))
        result |= VK_PIPELINE_STAGE_TRANSFER_BIT;
    if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(PipelineStage::Host))
        result |= VK_PIPELINE_STAGE_HOST_BIT;
    if (result == 0) result = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    return result;
}

static VkAccessFlags ToVkAccessFlags(AccessFlags access)
{
    VkAccessFlags result = 0;
    if (static_cast<uint32_t>(access) & static_cast<uint32_t>(AccessFlags::ShaderRead))
        result |= VK_ACCESS_SHADER_READ_BIT;
    if (static_cast<uint32_t>(access) & static_cast<uint32_t>(AccessFlags::ShaderWrite))
        result |= VK_ACCESS_SHADER_WRITE_BIT;
    if (static_cast<uint32_t>(access) & static_cast<uint32_t>(AccessFlags::ColorAttachmentWrite))
        result |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    if (static_cast<uint32_t>(access) & static_cast<uint32_t>(AccessFlags::UniformRead))
        result |= VK_ACCESS_UNIFORM_READ_BIT;
    if (static_cast<uint32_t>(access) & static_cast<uint32_t>(AccessFlags::TransferRead))
        result |= VK_ACCESS_TRANSFER_READ_BIT;
    if (static_cast<uint32_t>(access) & static_cast<uint32_t>(AccessFlags::TransferWrite))
        result |= VK_ACCESS_TRANSFER_WRITE_BIT;
    return result;
}

static VkImageLayout ToVkImageLayout(ImageLayout layout)
{
    switch (layout) {
        case ImageLayout::Undefined:             return VK_IMAGE_LAYOUT_UNDEFINED;
        case ImageLayout::General:               return VK_IMAGE_LAYOUT_GENERAL;
        case ImageLayout::ColorAttachment:       return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        case ImageLayout::DepthStencilAttachment:return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        case ImageLayout::ShaderRead:            return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case ImageLayout::TransferSrc:           return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        case ImageLayout::TransferDst:           return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        case ImageLayout::PresentSrc:            return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        default: return VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

// ============================================================
// Construction
// ============================================================

VKCommandBuffer::VKCommandBuffer(VKDevice& device, void* vkCommandBuffer, uint32_t frameIndex)
    : m_Device(device)
    , m_Resources(device.GetResourceManager())
    , m_VkCmd(vkCommandBuffer)
    , m_FrameIndex(frameIndex)
{
    m_DescriptorCache = &device.GetDescriptorSetCache();
}

VKCommandBuffer::~VKCommandBuffer()
{
    m_VkCmd = nullptr;
}

void VKCommandBuffer::ClearDescriptorSetCache()
{
    // Called from VKDevice::Shutdown() - cache cleared there
}

// ============================================================
// Lifecycle
// ============================================================

void VKCommandBuffer::Begin()
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(static_cast<VkCommandBuffer>(m_VkCmd), &beginInfo);
    m_IsRecording = true;
}

void VKCommandBuffer::End()
{
    if (m_InRenderPass) EndRenderPass();
    vkEndCommandBuffer(static_cast<VkCommandBuffer>(m_VkCmd));
    m_IsRecording = false;
}

CommandBufferLevel VKCommandBuffer::GetLevel() const { return CommandBufferLevel::Primary; }

// ============================================================
// Barriers
// ============================================================

void VKCommandBuffer::Barrier(PipelineStage srcStage, AccessFlags srcAccess,
                               PipelineStage dstStage, AccessFlags dstAccess)
{
    VkMemoryBarrier memoryBarrier{};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier.srcAccessMask = ToVkAccessFlags(srcAccess);
    memoryBarrier.dstAccessMask = ToVkAccessFlags(dstAccess);

    vkCmdPipelineBarrier(static_cast<VkCommandBuffer>(m_VkCmd),
                         ToVkPipelineStage(srcStage),
                         ToVkPipelineStage(dstStage),
                         0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
}

void VKCommandBuffer::Barrier(TextureHandle texture,
                               PipelineStage srcStage, AccessFlags srcAccess,
                               PipelineStage dstStage, AccessFlags dstAccess,
                               ImageLayout oldLayout, ImageLayout newLayout)
{
    auto* texRes = dynamic_cast<VKTextureResource*>(m_Resources.GetTexture(texture));
    if (!texRes || !texRes->GetVkImage()) return;

    VkImageMemoryBarrier imgBarrier{};
    imgBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imgBarrier.srcAccessMask = ToVkAccessFlags(srcAccess);
    imgBarrier.dstAccessMask = ToVkAccessFlags(dstAccess);
    imgBarrier.oldLayout = ToVkImageLayout(oldLayout);
    imgBarrier.newLayout = ToVkImageLayout(newLayout);
    imgBarrier.image = texRes->GetVkImage();
    imgBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imgBarrier.subresourceRange.baseMipLevel = 0;
    imgBarrier.subresourceRange.levelCount = 1;
    imgBarrier.subresourceRange.baseArrayLayer = 0;
    imgBarrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(static_cast<VkCommandBuffer>(m_VkCmd),
                         ToVkPipelineStage(srcStage),
                         ToVkPipelineStage(dstStage),
                         0, 0, nullptr, 0, nullptr, 1, &imgBarrier);
}

// ============================================================
// RenderPass
// ============================================================

void VKCommandBuffer::BeginRenderPass(RenderPassHandle renderPass, FramebufferHandle framebuffer,
                                       const Rect2D& renderArea,
                                       const ClearValue* clearValues, uint32_t clearValueCount)
{
    VkRenderPass rp = static_cast<VkRenderPass>(m_Device.GetRenderPass());
    if (rp == VK_NULL_HANDLE) { fprintf(stderr, "VK: invalid render pass\n"); return; }

    VkFramebuffer fb = static_cast<VkFramebuffer>(
        m_Device.GetSwapchainFramebuffer(m_Device.GetCurrentImageIndex()));
    if (fb == VK_NULL_HANDLE) { fprintf(stderr, "VK: invalid framebuffer\n"); return; }

    VkClearValue vkClearValues[2]{};
    for (uint32_t i = 0; i < std::min(clearValueCount, 2u); ++i) {
        vkClearValues[i].color.float32[0] = clearValues[i].Color.RGBA[0];
        vkClearValues[i].color.float32[1] = clearValues[i].Color.RGBA[1];
        vkClearValues[i].color.float32[2] = clearValues[i].Color.RGBA[2];
        vkClearValues[i].color.float32[3] = clearValues[i].Color.RGBA[3];
    }

    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass = rp;
    rpInfo.framebuffer = fb;
    rpInfo.renderArea.offset = {static_cast<int32_t>(renderArea.Offset.X), static_cast<int32_t>(renderArea.Offset.Y)};
    rpInfo.renderArea.extent = {renderArea.Extent.Width, renderArea.Extent.Height};
    rpInfo.clearValueCount = std::min(clearValueCount, 2u);
    rpInfo.pClearValues = vkClearValues;

    vkCmdBeginRenderPass(static_cast<VkCommandBuffer>(m_VkCmd), &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
    m_InRenderPass = true;

    VkViewport vp{};
    vp.x = 0; vp.y = 0;
    vp.width = static_cast<float>(renderArea.Extent.Width);
    vp.height = static_cast<float>(renderArea.Extent.Height);
    vp.minDepth = 0.0f; vp.maxDepth = 1.0f;
    vkCmdSetViewport(static_cast<VkCommandBuffer>(m_VkCmd), 0, 1, &vp);

    VkRect2D scissor{};
    scissor.offset = {static_cast<int32_t>(renderArea.Offset.X), static_cast<int32_t>(renderArea.Offset.Y)};
    scissor.extent = {renderArea.Extent.Width, renderArea.Extent.Height};
    vkCmdSetScissor(static_cast<VkCommandBuffer>(m_VkCmd), 0, 1, &scissor);
}

void VKCommandBuffer::EndRenderPass()
{
    vkCmdEndRenderPass(static_cast<VkCommandBuffer>(m_VkCmd));
    m_InRenderPass = false;
}

// ============================================================
// BindPipeline
// ============================================================

void VKCommandBuffer::BindPipeline(PipelineHandle pipeline)
{
    m_CurrentPipelineId = pipeline.GetId();
    m_CurrentPipeline = dynamic_cast<VKPipelineResource*>(m_Resources.GetPipeline(pipeline));
    if (!m_CurrentPipeline) return;

    vkCmdBindPipeline(static_cast<VkCommandBuffer>(m_VkCmd),
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      m_CurrentPipeline->GetVkPipeline());
}

// ============================================================
// BindGeometry
// ============================================================

void VKCommandBuffer::BindGeometry(GeometryHandle geometry)
{
    m_CurrentGeometryId = geometry.GetId();
    m_CurrentGeometry = dynamic_cast<VKGeometryResource*>(m_Resources.GetGeometry(geometry));
    if (!m_CurrentGeometry) return;

    const auto& geoDesc = m_CurrentGeometry->GetDesc();

    for (uint32_t i = 0; i < geoDesc.VertexBufferCount; ++i)
    {
        auto* bufRes = dynamic_cast<VKBufferResource*>(m_Resources.GetBuffer(geoDesc.VertexBuffers[i]));
        if (!bufRes) continue;
        VkBuffer vkBuf = bufRes->GetVkBuffer();
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(static_cast<VkCommandBuffer>(m_VkCmd), i, 1, &vkBuf, &offset);
    }

    if (geoDesc.IndexBuffer.IsValid())
    {
        auto* idxRes = dynamic_cast<VKBufferResource*>(m_Resources.GetBuffer(geoDesc.IndexBuffer));
        if (idxRes)
        {
            VkIndexType idxType = (geoDesc.IndexType == IndexType::UInt16) ?
                VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
            vkCmdBindIndexBuffer(static_cast<VkCommandBuffer>(m_VkCmd), idxRes->GetVkBuffer(), 0, idxType);
        }
    }
}

// ============================================================
// Descriptor Helpers
// ============================================================

void VKCommandBuffer::WriteDescriptorSet(uint32_t slot, uint32_t type,
                                          void* vkBuf, uint64_t offset, uint64_t range)
{
    if (!m_CurrentPipeline) return;

    // Allocate or reuse descriptor set (per pipeline, per in-flight frame)
    uint64_t cacheKey = (static_cast<uint64_t>(m_CurrentPipelineId) << 8) | m_FrameIndex;
    auto& cache = m_Device.GetDescriptorSetCache();
    VkDescriptorSet ds = VK_NULL_HANDLE;
    auto cacheIt = cache.find(cacheKey);
    if (cacheIt != cache.end())
    {
        ds = static_cast<VkDescriptorSet>(cacheIt->second);
    }
    else
    {
        ds = static_cast<VkDescriptorSet>(
            m_Device.AllocateDescriptorSet(m_CurrentPipeline->GetVkDescriptorSetLayout()));
        if (!ds) return;
        cache[cacheKey] = ds;
    }

    VkDescriptorBufferInfo bufInfo{};
    bufInfo.buffer = static_cast<VkBuffer>(vkBuf);
    bufInfo.offset = offset;
    bufInfo.range = (range > 0) ? range : VK_WHOLE_SIZE;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = ds;
    write.dstBinding = slot;
    write.dstArrayElement = 0;
    write.descriptorCount = 1;
    write.descriptorType = static_cast<VkDescriptorType>(type);
    write.pBufferInfo = &bufInfo;

    vkUpdateDescriptorSets(static_cast<VkDevice>(m_Device.GetVkDevice()), 1, &write, 0, nullptr);

    vkCmdBindDescriptorSets(static_cast<VkCommandBuffer>(m_VkCmd),
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_CurrentPipeline->GetVkPipelineLayout(),
                            0, 1, &ds, 0, nullptr);
}

// ============================================================
// Buffer/Texture Binding
// ============================================================

void VKCommandBuffer::BindUniformBuffer(uint32_t slot, BufferHandle buffer,
                                         uint64_t offset, uint64_t range)
{
    auto* bufRes = dynamic_cast<VKBufferResource*>(m_Resources.GetBuffer(buffer));
    if (!bufRes) return;
    WriteDescriptorSet(slot, static_cast<uint32_t>(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
                       static_cast<void*>(bufRes->GetVkBuffer()), offset, range);
}

void VKCommandBuffer::BindStorageBuffer(uint32_t slot, BufferHandle buffer,
                                         uint64_t offset, uint64_t range)
{
    auto* bufRes = dynamic_cast<VKBufferResource*>(m_Resources.GetBuffer(buffer));
    if (!bufRes) return;
    WriteDescriptorSet(slot, static_cast<uint32_t>(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
                       static_cast<void*>(bufRes->GetVkBuffer()), offset, range);
}

void VKCommandBuffer::BindTexture(uint32_t, TextureHandle, SamplerHandle)
{
    // [HACK: texture descriptor writes require image+sampler info from VKTextureResource]
}

void VKCommandBuffer::BindStorageTexture(uint32_t, TextureHandle)
{
    // [HACK: storage image descriptor writes require VKTextureResource VkImageView]
}

// ============================================================
// PushConstants
// ============================================================

void VKCommandBuffer::PushConstants(const void* data, uint32_t size, uint32_t offset)
{
    if (!m_CurrentPipeline) return;
    vkCmdPushConstants(static_cast<VkCommandBuffer>(m_VkCmd),
                       m_CurrentPipeline->GetVkPipelineLayout(),
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       offset, size, data);
}

// ============================================================
// Draw
// ============================================================

void VKCommandBuffer::Draw(uint32_t vertexCount, uint32_t firstVertex, uint32_t instanceCount)
{
    vkCmdDraw(static_cast<VkCommandBuffer>(m_VkCmd), vertexCount, instanceCount, firstVertex, 0);
}

void VKCommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t firstIndex,
                                   int32_t vertexOffset, uint32_t instanceCount)
{
    vkCmdDrawIndexed(static_cast<VkCommandBuffer>(m_VkCmd), indexCount, instanceCount,
                     firstIndex, vertexOffset, 0);
}

void VKCommandBuffer::DrawIndirect(BufferHandle indirectBuffer, uint64_t offset,
                                    uint32_t drawCount, uint32_t stride)
{
    auto* bufRes = dynamic_cast<VKBufferResource*>(m_Resources.GetBuffer(indirectBuffer));
    if (!bufRes) return;
    vkCmdDrawIndirect(static_cast<VkCommandBuffer>(m_VkCmd), bufRes->GetVkBuffer(),
                      offset, drawCount, stride);
}

// ============================================================
// Compute
// ============================================================

void VKCommandBuffer::DispatchCompute(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    vkCmdDispatch(static_cast<VkCommandBuffer>(m_VkCmd), groupX, groupY, groupZ);
}

void VKCommandBuffer::DispatchIndirect(BufferHandle indirectBuffer, uint64_t offset)
{
    auto* bufRes = dynamic_cast<VKBufferResource*>(m_Resources.GetBuffer(indirectBuffer));
    if (!bufRes) return;
    vkCmdDispatchIndirect(static_cast<VkCommandBuffer>(m_VkCmd), bufRes->GetVkBuffer(), offset);
}

// ============================================================
// Copy (stubs)
// ============================================================

void VKCommandBuffer::CopyBuffer(BufferHandle, BufferHandle, uint64_t, uint64_t, uint64_t) {}
void VKCommandBuffer::CopyBufferToTexture(BufferHandle, TextureHandle, const Offset3D&, const Extent3D&) {}
void VKCommandBuffer::CopyTextureToBuffer(TextureHandle, BufferHandle, const Offset3D&, const Extent3D&) {}

// ============================================================
// GPU Queries / Timestamps
// ============================================================

void VKCommandBuffer::ResetQueryPool(QueryPoolHandle pool, uint32_t firstQuery, uint32_t queryCount)
{
    VkQueryPool vkPool = static_cast<VkQueryPool>(m_Device.GetQueryPoolVK(pool.GetId()));
    if (vkPool)
        vkCmdResetQueryPool(static_cast<VkCommandBuffer>(m_VkCmd), vkPool, firstQuery, queryCount);
}

void VKCommandBuffer::BeginQuery(QueryPoolHandle pool, uint32_t queryIndex)
{
    VkQueryPool vkPool = static_cast<VkQueryPool>(m_Device.GetQueryPoolVK(pool.GetId()));
    if (vkPool)
        vkCmdBeginQuery(static_cast<VkCommandBuffer>(m_VkCmd), vkPool, queryIndex, 0);
}

void VKCommandBuffer::EndQuery(QueryPoolHandle pool, uint32_t queryIndex)
{
    VkQueryPool vkPool = static_cast<VkQueryPool>(m_Device.GetQueryPoolVK(pool.GetId()));
    if (vkPool)
        vkCmdEndQuery(static_cast<VkCommandBuffer>(m_VkCmd), vkPool, queryIndex);
}

void VKCommandBuffer::WriteTimestamp(PipelineStage stage, QueryPoolHandle pool, uint32_t queryIndex)
{
    VkQueryPool vkPool = static_cast<VkQueryPool>(m_Device.GetQueryPoolVK(pool.GetId()));
    if (vkPool)
        vkCmdWriteTimestamp(static_cast<VkCommandBuffer>(m_VkCmd),
                            static_cast<VkPipelineStageFlagBits>(ToVkPipelineStage(stage)),
                            vkPool, queryIndex);
}

// ============================================================
// Debug Markers (VK_EXT_debug_utils)
// ============================================================

void VKCommandBuffer::BeginDebugLabel(const char* label, std::array<float, 4> color)
{
    auto vkCmdBeginDebugUtilsLabel = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(
        m_Device.GetDebugUtilsLabelBegin());
    if (vkCmdBeginDebugUtilsLabel)
    {
        VkDebugUtilsLabelEXT labelInfo{};
        labelInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        labelInfo.pLabelName = label;
        labelInfo.color[0] = color[0]; labelInfo.color[1] = color[1];
        labelInfo.color[2] = color[2]; labelInfo.color[3] = color[3];
        vkCmdBeginDebugUtilsLabel(static_cast<VkCommandBuffer>(m_VkCmd), &labelInfo);
    }
}

void VKCommandBuffer::EndDebugLabel()
{
    auto vkCmdEndDebugUtilsLabel = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(
        m_Device.GetDebugUtilsLabelEnd());
    if (vkCmdEndDebugUtilsLabel)
        vkCmdEndDebugUtilsLabel(static_cast<VkCommandBuffer>(m_VkCmd));
}

void VKCommandBuffer::InsertDebugMarker(const char* marker)
{
    auto vkCmdInsertDebugUtilsLabel = reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(
        m_Device.GetDebugUtilsLabelInsert());
    if (vkCmdInsertDebugUtilsLabel)
    {
        VkDebugUtilsLabelEXT labelInfo{};
        labelInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        labelInfo.pLabelName = marker;
        vkCmdInsertDebugUtilsLabel(static_cast<VkCommandBuffer>(m_VkCmd), &labelInfo);
    }
}

} // namespace rhi
