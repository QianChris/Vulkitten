#include "vktpch.h"
#include "GLCommandBuffer.h"

#include "OpenGLDevice.h"
#include "Vulkitten/Renderer/Device.h"

#include <glad/glad.h>

namespace Vulkitten {

GLCommandBuffer::GLCommandBuffer(OpenGLDevice& device)
    : m_Device(device)
{
}

GLCommandBuffer::~GLCommandBuffer()
{
}

void GLCommandBuffer::Begin()
{
    m_CurrentPipelineId = 0;
    m_CurrentGeometryId = 0;
    m_CurrentFbo = 0;
    m_InRenderPass = false;
}

void GLCommandBuffer::End()
{
    if (m_InRenderPass)
        EndRenderPass();
}

// ---- Barriers ----

void GLCommandBuffer::Barrier(rhi::PipelineStage, rhi::AccessFlags,
                               rhi::PipelineStage, rhi::AccessFlags)
{
}

void GLCommandBuffer::Barrier(rhi::TextureHandle,
                               rhi::PipelineStage, rhi::AccessFlags,
                               rhi::PipelineStage, rhi::AccessFlags,
                               rhi::ImageLayout, rhi::ImageLayout)
{
}

// ---- RenderPass ----

void GLCommandBuffer::BeginRenderPass(rhi::RenderPassHandle /*renderPass*/,
                                       rhi::FramebufferHandle framebuffer,
                                       const rhi::Rect2D& /*renderArea*/,
                                       const rhi::ClearValue* clearValues,
                                       uint32_t clearValueCount)
{
    m_InRenderPass = true;

    if (framebuffer.IsValid())
    {
        // [HACK: resolve FBO from device slot — Task 14 Pass integration]
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        m_CurrentFbo = 0;
    }
    else
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        m_CurrentFbo = 0;
    }

    GLbitfield clearMask = 0;
    for (uint32_t i = 0; i < clearValueCount; i++)
    {
        glClearColor(clearValues[i].Color.RGBA[0], clearValues[i].Color.RGBA[1],
                     clearValues[i].Color.RGBA[2], clearValues[i].Color.RGBA[3]);
        clearMask |= GL_COLOR_BUFFER_BIT;
    }
    if (clearMask)
        glClear(clearMask);
}

void GLCommandBuffer::EndRenderPass()
{
    // [HACK: glBindFramebuffer(0) — Task 14]
    m_InRenderPass = false;
}

// ---- Pipeline & Geometry ----

void GLCommandBuffer::BindPipeline(rhi::PipelineHandle pipeline)
{
    if (!pipeline.IsValid())
        return;
    uint32_t pid = pipeline.GetId();
    if (pid == m_CurrentPipelineId)
        return;
    m_CurrentPipelineId = pid;

    // [HACK: resolve GL program from device slot — Task 14]
}

void GLCommandBuffer::BindGeometry(rhi::GeometryHandle geometry)
{
    if (!geometry.IsValid())
        return;
    uint32_t gid = geometry.GetId();
    if (gid == m_CurrentGeometryId)
        return;
    m_CurrentGeometryId = gid;

    // [HACK: lazy VAO creation from Pipeline vertexLayout + Geometry buffers — Task 14]
}

// ---- Descriptors ----

void GLCommandBuffer::BindUniformBuffer(uint32_t /*slot*/, rhi::BufferHandle /*buffer*/,
                                         uint64_t /*offset*/, uint64_t /*range*/)
{
}

void GLCommandBuffer::BindStorageBuffer(uint32_t /*slot*/, rhi::BufferHandle /*buffer*/,
                                         uint64_t /*offset*/, uint64_t /*range*/)
{
}

void GLCommandBuffer::BindTexture(uint32_t /*slot*/, rhi::TextureHandle /*texture*/,
                                   rhi::SamplerHandle /*sampler*/)
{
    // [HACK: glActiveTexture + glBindTexture — Task 14]
}

void GLCommandBuffer::BindStorageTexture(uint32_t /*slot*/, rhi::TextureHandle /*texture*/)
{
}

// ---- Push Constants ----

void GLCommandBuffer::PushConstants(const void* data, uint32_t size, uint32_t offset)
{
    // [HACK: glUniformMatrix4fv for VP matrix — Task 14 refactors SpriteRenderPass]
    (void)data; (void)size; (void)offset;
}

// ---- Draw ----

void GLCommandBuffer::Draw(uint32_t vertexCount, uint32_t firstVertex, uint32_t instanceCount)
{
    // [HACK: glDrawArrays — Task 14 refactors SpriteRenderPass]
    (void)vertexCount; (void)firstVertex; (void)instanceCount;
}

void GLCommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t firstIndex,
                                   int32_t vertexOffset, uint32_t instanceCount)
{
    // [HACK: glDrawElements — Task 14 refactors SpriteRenderPass]
    (void)indexCount; (void)firstIndex; (void)vertexOffset; (void)instanceCount;
}

// ---- Compute ----

void GLCommandBuffer::DispatchCompute(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    glDispatchCompute(groupX, groupY, groupZ);
}

// ---- Copy ----

void GLCommandBuffer::CopyBuffer(rhi::BufferHandle, rhi::BufferHandle,
                                  uint64_t, uint64_t, uint64_t) {}
void GLCommandBuffer::CopyBufferToTexture(rhi::BufferHandle, rhi::TextureHandle,
                                           const rhi::Offset3D&, const rhi::Extent3D&) {}
void GLCommandBuffer::CopyTextureToBuffer(rhi::TextureHandle, rhi::BufferHandle,
                                           const rhi::Offset3D&, const rhi::Extent3D&) {}

// ---- Debug ----

void GLCommandBuffer::BeginDebugLabel(const char* label, std::array<float, 4>)
{
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, label);
}

void GLCommandBuffer::EndDebugLabel()
{
    glPopDebugGroup();
}

void GLCommandBuffer::InsertDebugMarker(const char*) {}

} // namespace Vulkitten
