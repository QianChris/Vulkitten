#include "VKCommandBuffer.hpp"
#include "VKDevice.hpp"
#include "VKSwapchain.hpp"

#include <vulkan/vulkan.h>

#include <cstring>
#include <cstdio>

namespace rhi {

// ============================================================
// Construction
// ============================================================

VKCommandBuffer::VKCommandBuffer(VKDevice& device, void* vkCommandBuffer)
    : m_Device(device)
    , m_VkCmd(vkCommandBuffer)
{
}

VKCommandBuffer::~VKCommandBuffer()
{
    // VkCommandBuffer is freed when the command pool is reset/destroyed
    m_VkCmd = nullptr;
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
    if (m_InRenderPass)
    {
        EndRenderPass();
    }

    vkEndCommandBuffer(static_cast<VkCommandBuffer>(m_VkCmd));
    m_IsRecording = false;

    // Submission is done by VKDevice::EndFrame() with proper sync objects
    // (image-available semaphore wait → submit → render-finished semaphore signal → present)
}

CommandBufferLevel VKCommandBuffer::GetLevel() const
{
    return CommandBufferLevel::Primary;
}

// ============================================================
// Barriers
// ============================================================

void VKCommandBuffer::Barrier(PipelineStage /*srcStage*/, AccessFlags /*srcAccess*/,
                               PipelineStage /*dstStage*/, AccessFlags /*dstAccess*/)
{
    // [STUB: MVP clear-to-red doesn't need barriers]
}

void VKCommandBuffer::Barrier(TextureHandle /*texture*/,
                               PipelineStage /*srcStage*/, AccessFlags /*srcAccess*/,
                               PipelineStage /*dstStage*/, AccessFlags /*dstAccess*/,
                               ImageLayout /*oldLayout*/, ImageLayout /*newLayout*/)
{
    // [STUB]
}

// ============================================================
// RenderPass
// ============================================================

void VKCommandBuffer::BeginRenderPass(RenderPassHandle renderPass, FramebufferHandle framebuffer,
                                       const Rect2D& renderArea,
                                       const ClearValue* clearValues, uint32_t clearValueCount)
{
    VkRenderPass rp = VK_NULL_HANDLE;

    // Get render pass from device's handle pool
    auto* rpSlot = m_Device.GetSlot(renderPass.GetId());
    if (rpSlot && rpSlot->Alive)
        rp = reinterpret_cast<VkRenderPass>(rpSlot->GpuHandle);

    if (rp == VK_NULL_HANDLE)
    {
        fprintf(stderr, "VKCommandBuffer::BeginRenderPass: invalid render pass\n");
        return;
    }

    // Framebuffer: resolve from handle. If it's the swapchain sentinel,
    // fetch the correct per-image VkFramebuffer from VKSwapchain.
    VkFramebuffer fb = VK_NULL_HANDLE;
    auto* fbSlot = m_Device.GetSlot(framebuffer.GetId());
    if (fbSlot && fbSlot->Alive)
    {
        if (fbSlot->GpuHandle == VKDevice::kSwapchainFramebufferSentinel)
        {
            // Swapchain-owned: resolve to the framebuffer for the current image
            fb = static_cast<VkFramebuffer>(
                m_Device.GetSwapchainFramebuffer(m_Device.GetCurrentImageIndex()));
        }
        else
        {
            fb = reinterpret_cast<VkFramebuffer>(fbSlot->GpuHandle);
        }
    }

    if (fb == VK_NULL_HANDLE)
    {
        fprintf(stderr, "VKCommandBuffer::BeginRenderPass: invalid framebuffer\n");
        return;
    }

    // Prepare clear values for Vulkan
    VkClearValue vkClearValues[2]{};
    uint32_t vkClearCount = std::min(clearValueCount, 2u);
    for (uint32_t i = 0; i < vkClearCount; ++i)
    {
        vkClearValues[i].color.float32[0] = clearValues[i].Color.RGBA[0];
        vkClearValues[i].color.float32[1] = clearValues[i].Color.RGBA[1];
        vkClearValues[i].color.float32[2] = clearValues[i].Color.RGBA[2];
        vkClearValues[i].color.float32[3] = clearValues[i].Color.RGBA[3];
    }

    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass = rp;
    rpInfo.framebuffer = fb;
    rpInfo.renderArea.offset = {static_cast<int32_t>(renderArea.Offset.X),
                                 static_cast<int32_t>(renderArea.Offset.Y)};
    rpInfo.renderArea.extent = {renderArea.Extent.Width, renderArea.Extent.Height};
    rpInfo.clearValueCount = vkClearCount;
    rpInfo.pClearValues = vkClearValues;

    vkCmdBeginRenderPass(static_cast<VkCommandBuffer>(m_VkCmd), &rpInfo,
                          VK_SUBPASS_CONTENTS_INLINE);
    m_InRenderPass = true;
}

void VKCommandBuffer::EndRenderPass()
{
    vkCmdEndRenderPass(static_cast<VkCommandBuffer>(m_VkCmd));
    m_InRenderPass = false;
}

// ============================================================
// Binding (stubs for MVP)
// ============================================================

void VKCommandBuffer::BindPipeline(PipelineHandle /*pipeline*/) { /* [STUB] */ }
void VKCommandBuffer::BindGeometry(GeometryHandle /*geometry*/) { /* [STUB] */ }
void VKCommandBuffer::BindUniformBuffer(uint32_t, BufferHandle, uint64_t, uint64_t) { /* [STUB] */ }
void VKCommandBuffer::BindStorageBuffer(uint32_t, BufferHandle, uint64_t, uint64_t) { /* [STUB] */ }
void VKCommandBuffer::BindTexture(uint32_t, TextureHandle, SamplerHandle) { /* [STUB] */ }
void VKCommandBuffer::BindStorageTexture(uint32_t, TextureHandle) { /* [STUB] */ }
void VKCommandBuffer::PushConstants(const void*, uint32_t, uint32_t) { /* [STUB] */ }

// ============================================================
// Draw (stubs)
// ============================================================

void VKCommandBuffer::Draw(uint32_t, uint32_t, uint32_t) { /* [STUB] */ }
void VKCommandBuffer::DrawIndexed(uint32_t, uint32_t, int32_t, uint32_t) { /* [STUB] */ }

// ============================================================
// Compute (stub)
// ============================================================

void VKCommandBuffer::DispatchCompute(uint32_t, uint32_t, uint32_t) { /* [STUB] */ }

// ============================================================
// Copy (stubs)
// ============================================================

void VKCommandBuffer::CopyBuffer(BufferHandle, BufferHandle, uint64_t, uint64_t, uint64_t) { /* [STUB] */ }
void VKCommandBuffer::CopyBufferToTexture(BufferHandle, TextureHandle, const Offset3D&, const Extent3D&) { /* [STUB] */ }
void VKCommandBuffer::CopyTextureToBuffer(TextureHandle, BufferHandle, const Offset3D&, const Extent3D&) { /* [STUB] */ }

// ============================================================
// Debug
// ============================================================

void VKCommandBuffer::BeginDebugLabel(const char* /*label*/, std::array<float, 4> /*color*/)
{
    // [STUB: requires VK_EXT_debug_utils]
}

void VKCommandBuffer::EndDebugLabel()
{
    // [STUB]
}

void VKCommandBuffer::InsertDebugMarker(const char* /*marker*/)
{
    // [STUB]
}

} // namespace rhi
