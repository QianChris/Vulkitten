#include "GLCommandBuffer.hpp"
#include "GLDevice.hpp"

#include <glad/glad.h>

#include <cstring>
#include <cstdio>

namespace rhi {

// ============================================================
// Construction
// ============================================================

GLCommandBuffer::GLCommandBuffer(GLDevice& device)
    : m_Device(device)
{
}

GLCommandBuffer::~GLCommandBuffer()
{
    // Clean up cached VAOs
    for (auto& [key, vao] : m_VaoCache)
    {
        glDeleteVertexArrays(1, &vao);
    }
    m_VaoCache.clear();
}

// ============================================================
// Lifecycle
// ============================================================

void GLCommandBuffer::Begin()
{
    // GL immediate mode: no begin needed
    m_InRenderPass = false;
}

void GLCommandBuffer::End()
{
    // GL immediate mode: no end needed
    if (m_InRenderPass)
    {
        EndRenderPass();
    }
}

CommandBufferLevel GLCommandBuffer::GetLevel() const
{
    return CommandBufferLevel::Primary;
}

// ============================================================
// Barriers (GL: mostly no-op except texture barrier)
// ============================================================

void GLCommandBuffer::Barrier(PipelineStage /*srcStage*/, AccessFlags /*srcAccess*/,
                               PipelineStage /*dstStage*/, AccessFlags /*dstAccess*/)
{
    // GL implicit sync; no-op for barriers between draw calls
}

void GLCommandBuffer::Barrier(TextureHandle /*texture*/,
                               PipelineStage /*srcStage*/, AccessFlags /*srcAccess*/,
                               PipelineStage /*dstStage*/, AccessFlags /*dstAccess*/,
                               ImageLayout /*oldLayout*/, ImageLayout /*newLayout*/)
{
    // GL implicit sync; glTextureBarrier for feedback loops if needed
}

// ============================================================
// RenderPass
// ============================================================

void GLCommandBuffer::BeginRenderPass(RenderPassHandle renderPass, FramebufferHandle framebuffer,
                                       const Rect2D& /*renderArea*/,
                                       const ClearValue* clearValues, uint32_t clearValueCount)
{
    m_InRenderPass = true;

    // Bind the framebuffer (0 = default/swapchain)
    GLuint fbo = 0;
    if (framebuffer.IsValid())
    {
        auto* fbslot = m_Device.GetSlot(framebuffer.GetId());
        if (fbslot && fbslot->Alive)
            fbo = static_cast<GLuint>(fbslot->GpuHandle);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    m_CurrentFbo = fbo;

    // Apply clear values
    GLbitfield clearMask = 0;

    // Read the render pass descriptor for attachment count info
    auto* rpDesc = m_Device.GetRenderPassDesc(renderPass.GetId());
    uint32_t numColorAttachments = rpDesc ? static_cast<uint32_t>(rpDesc->ColorAttachments.size()) : 1;

    for (uint32_t i = 0; i < clearValueCount && i < numColorAttachments; ++i)
    {
        const auto& cv = clearValues[i];
        glClearColor(cv.Color.RGBA[0], cv.Color.RGBA[1],
                     cv.Color.RGBA[2], cv.Color.RGBA[3]);
        clearMask |= GL_COLOR_BUFFER_BIT;
    }

    // Check for depth attachment
    bool hasDepth = rpDesc && rpDesc->DepthStencilAttachment.Format != Format::Unknown;
    if (hasDepth && clearValueCount > numColorAttachments)
    {
        const auto& cv = clearValues[numColorAttachments];
        glClearDepthf(cv.DepthStencil.Depth);
        clearMask |= GL_DEPTH_BUFFER_BIT;
    }
    else if (hasDepth)
    {
        glClearDepthf(1.0f);
        clearMask |= GL_DEPTH_BUFFER_BIT;
    }

    if (clearMask != 0)
        glClear(clearMask);
}

void GLCommandBuffer::EndRenderPass()
{
    m_InRenderPass = false;
    // Unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    m_CurrentFbo = 0;
}

// ============================================================
// Binding
// ============================================================

void GLCommandBuffer::BindPipeline(PipelineHandle pipeline)
{
    // [STUB: MVP clear-to-red doesn't use pipelines]
    m_CurrentPipelineId = pipeline.GetId();
}

void GLCommandBuffer::BindGeometry(GeometryHandle geometry)
{
    // [STUB: MVP clear-to-red doesn't use geometry]
    m_CurrentGeometryId = geometry.GetId();
}

void GLCommandBuffer::BindUniformBuffer(uint32_t /*slot*/, BufferHandle /*buffer*/,
                                         uint64_t /*offset*/, uint64_t /*range*/)
{
    // [STUB]
}

void GLCommandBuffer::BindStorageBuffer(uint32_t /*slot*/, BufferHandle /*buffer*/,
                                         uint64_t /*offset*/, uint64_t /*range*/)
{
    // [STUB]
}

void GLCommandBuffer::BindTexture(uint32_t /*slot*/, TextureHandle /*texture*/,
                                   SamplerHandle /*sampler*/)
{
    // [STUB]
}

void GLCommandBuffer::BindStorageTexture(uint32_t /*slot*/, TextureHandle /*texture*/)
{
    // [STUB]
}

void GLCommandBuffer::PushConstants(const void* /*data*/, uint32_t /*size*/, uint32_t /*offset*/)
{
    // [STUB]
}

// ============================================================
// Draw
// ============================================================

void GLCommandBuffer::Draw(uint32_t /*vertexCount*/, uint32_t /*firstVertex*/,
                            uint32_t /*instanceCount*/)
{
    // [STUB]
}

void GLCommandBuffer::DrawIndexed(uint32_t /*indexCount*/, uint32_t /*firstIndex*/,
                                   int32_t /*vertexOffset*/, uint32_t /*instanceCount*/)
{
    // [STUB]
}

// ============================================================
// Compute
// ============================================================

void GLCommandBuffer::DispatchCompute(uint32_t /*groupX*/, uint32_t /*groupY*/, uint32_t /*groupZ*/)
{
    // [STUB]
}

// ============================================================
// Copy
// ============================================================

void GLCommandBuffer::CopyBuffer(BufferHandle /*src*/, BufferHandle /*dst*/,
                                  uint64_t /*srcOffset*/, uint64_t /*dstOffset*/, uint64_t /*size*/)
{
    // [STUB]
}

void GLCommandBuffer::CopyBufferToTexture(BufferHandle /*src*/, TextureHandle /*dst*/,
                                           const Offset3D& /*dstOffset*/, const Extent3D& /*dstExtent*/)
{
    // [STUB]
}

void GLCommandBuffer::CopyTextureToBuffer(TextureHandle /*src*/, BufferHandle /*dst*/,
                                           const Offset3D& /*srcOffset*/, const Extent3D& /*srcExtent*/)
{
    // [STUB]
}

// ============================================================
// Debug
// ============================================================

void GLCommandBuffer::BeginDebugLabel(const char* /*label*/,
                                       std::array<float, 4> /*color*/)
{
    // GL: no debug label support without extension
}

void GLCommandBuffer::EndDebugLabel()
{
    // GL: no-op
}

void GLCommandBuffer::InsertDebugMarker(const char* /*marker*/)
{
    // GL: no-op
}

} // namespace rhi
