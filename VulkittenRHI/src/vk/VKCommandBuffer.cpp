#include "VKCommandBuffer.hpp"
#include "VKDevice.hpp"
#include "VKResources.hpp"
#include "VKSwapchain.hpp"
#include "rhi/ResourceManager.hpp"

#include <vulkan/vulkan.h>

#include <cstring>
#include <cstdio>
#include <unordered_map>

namespace rhi {

// Per-pipeline, per-frame descriptor set cache
// Key: (pipelineId << 8) | frameIndex
static std::unordered_map<uint64_t, void*> s_DescriptorSetCache;

// ============================================================
// Construction
// ============================================================

VKCommandBuffer::VKCommandBuffer(VKDevice& device, void* vkCommandBuffer, uint32_t frameIndex)
    : m_Device(device)
    , m_Resources(device.GetResourceManager())
    , m_VkCmd(vkCommandBuffer)
    , m_FrameIndex(frameIndex)
{
}

VKCommandBuffer::~VKCommandBuffer()
{
    m_VkCmd = nullptr;
}

void VKCommandBuffer::ClearDescriptorSetCache()
{
    s_DescriptorSetCache.clear();
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
// Barriers (stubs)
// ============================================================

void VKCommandBuffer::Barrier(PipelineStage, AccessFlags, PipelineStage, AccessFlags) {}
void VKCommandBuffer::Barrier(TextureHandle, PipelineStage, AccessFlags,
                               PipelineStage, AccessFlags, ImageLayout, ImageLayout) {}

// ============================================================
// RenderPass
// ============================================================

void VKCommandBuffer::BeginRenderPass(RenderPassHandle renderPass, FramebufferHandle framebuffer,
                                       const Rect2D& renderArea,
                                       const ClearValue* clearValues, uint32_t clearValueCount)
{
    // Look up VkRenderPass — for now it's the swapchain's default render pass
    VkRenderPass rp = static_cast<VkRenderPass>(m_Device.GetRenderPass());
    if (rp == VK_NULL_HANDLE) { fprintf(stderr, "VK: invalid render pass\n"); return; }

    // Look up VkFramebuffer
    VkFramebuffer fb = VK_NULL_HANDLE;
    if (framebuffer.IsValid())
    {
        fb = static_cast<VkFramebuffer>(m_Device.GetSwapchainFramebuffer(m_Device.GetCurrentImageIndex()));
    }
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

    // Set viewport and scissor
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

    auto* pipeRes = dynamic_cast<VKPipelineResource*>(m_Resources.GetPipeline(pipeline));
    m_CurrentPipeline = pipeRes;

    if (!pipeRes) return;

    vkCmdBindPipeline(static_cast<VkCommandBuffer>(m_VkCmd),
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      pipeRes->GetVkPipeline());
}

// ============================================================
// BindGeometry
// ============================================================

void VKCommandBuffer::BindGeometry(GeometryHandle geometry)
{
    m_CurrentGeometryId = geometry.GetId();

    auto* geoRes = dynamic_cast<VKGeometryResource*>(m_Resources.GetGeometry(geometry));
    m_CurrentGeometry = geoRes;

    if (!geoRes) return;

    const auto& geoDesc = geoRes->GetDesc();

    // Bind vertex buffers
    for (uint32_t i = 0; i < geoDesc.VertexBufferCount; ++i)
    {
        auto* bufRes = dynamic_cast<VKBufferResource*>(
            m_Resources.GetBuffer(geoDesc.VertexBuffers[i]));
        if (!bufRes) continue;

        VkBuffer vkBuf = bufRes->GetVkBuffer();
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(static_cast<VkCommandBuffer>(m_VkCmd), i, 1, &vkBuf, &offset);
    }

    // Bind index buffer if present
    if (geoDesc.IndexBuffer.IsValid())
    {
        auto* idxRes = dynamic_cast<VKBufferResource*>(
            m_Resources.GetBuffer(geoDesc.IndexBuffer));
        if (idxRes)
        {
            VkIndexType idxType = (geoDesc.IndexType == IndexType::UInt16) ?
                VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
            vkCmdBindIndexBuffer(static_cast<VkCommandBuffer>(m_VkCmd),
                                 idxRes->GetVkBuffer(), 0, idxType);
        }
    }
}

// ============================================================
// BindUniformBuffer
// ============================================================

void VKCommandBuffer::BindUniformBuffer(uint32_t slot, BufferHandle buffer,
                                         uint64_t offset, uint64_t range)
{
    auto* pipeRes = m_CurrentPipeline;
    auto* bufRes = dynamic_cast<VKBufferResource*>(m_Resources.GetBuffer(buffer));
    if (!pipeRes || !bufRes) return;

    auto dev = static_cast<VkDevice>(m_Device.GetVkDevice());

    // Allocate or reuse descriptor set (per pipeline, per in-flight frame)
    uint64_t cacheKey = (static_cast<uint64_t>(m_CurrentPipelineId) << 8) | m_FrameIndex;
    VkDescriptorSet ds = VK_NULL_HANDLE;
    auto cacheIt = s_DescriptorSetCache.find(cacheKey);
    if (cacheIt != s_DescriptorSetCache.end())
    {
        ds = static_cast<VkDescriptorSet>(cacheIt->second);
    }
    else
    {
        ds = static_cast<VkDescriptorSet>(
            m_Device.AllocateDescriptorSet(pipeRes->GetVkDescriptorSetLayout()));
        if (!ds) return;
        s_DescriptorSetCache[cacheKey] = ds;
    }

    // Write descriptor
    VkDescriptorBufferInfo bufInfo{};
    bufInfo.buffer = bufRes->GetVkBuffer();
    bufInfo.offset = offset;
    bufInfo.range = (range > 0) ? range : VK_WHOLE_SIZE;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = ds;
    write.dstBinding = slot;
    write.dstArrayElement = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.pBufferInfo = &bufInfo;

    vkUpdateDescriptorSets(dev, 1, &write, 0, nullptr);

    // Bind descriptor set
    vkCmdBindDescriptorSets(static_cast<VkCommandBuffer>(m_VkCmd),
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeRes->GetVkPipelineLayout(),
                            0, 1, &ds, 0, nullptr);
}

void VKCommandBuffer::BindStorageBuffer(uint32_t slot, BufferHandle buffer,
                                         uint64_t offset, uint64_t range)
{
    BindUniformBuffer(slot, buffer, offset, range);  // same path for MVP
}

// ============================================================
// Texture binding (stubs)
// ============================================================

void VKCommandBuffer::BindTexture(uint32_t, TextureHandle, SamplerHandle) {}
void VKCommandBuffer::BindStorageTexture(uint32_t, TextureHandle) {}

// ============================================================
// PushConstants
// ============================================================

void VKCommandBuffer::PushConstants(const void* data, uint32_t size, uint32_t offset)
{
    auto* pipeRes = m_CurrentPipeline;
    if (!pipeRes) return;

    vkCmdPushConstants(static_cast<VkCommandBuffer>(m_VkCmd),
                       pipeRes->GetVkPipelineLayout(),
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

// ============================================================
// Compute (stub)
// ============================================================

void VKCommandBuffer::DispatchCompute(uint32_t, uint32_t, uint32_t) {}

// ============================================================
// Copy (stubs)
// ============================================================

void VKCommandBuffer::CopyBuffer(BufferHandle, BufferHandle, uint64_t, uint64_t, uint64_t) {}
void VKCommandBuffer::CopyBufferToTexture(BufferHandle, TextureHandle, const Offset3D&, const Extent3D&) {}
void VKCommandBuffer::CopyTextureToBuffer(TextureHandle, BufferHandle, const Offset3D&, const Extent3D&) {}

// ============================================================
// Debug (stubs)
// ============================================================

void VKCommandBuffer::BeginDebugLabel(const char*, std::array<float, 4>) {}
void VKCommandBuffer::EndDebugLabel() {}
void VKCommandBuffer::InsertDebugMarker(const char*) {}

} // namespace rhi
