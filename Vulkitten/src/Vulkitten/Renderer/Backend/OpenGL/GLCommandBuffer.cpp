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
    m_InRenderPass = false;
}

// ---- Barriers (no-ops for OpenGL) ----

void GLCommandBuffer::Barrier(rhi::PipelineStage, rhi::AccessFlags,
                               rhi::PipelineStage, rhi::AccessFlags)
{
    // OpenGL: implicit synchronization, no barrier needed
}

void GLCommandBuffer::Barrier(rhi::TextureHandle,
                               rhi::PipelineStage, rhi::AccessFlags,
                               rhi::PipelineStage, rhi::AccessFlags,
                               rhi::ImageLayout, rhi::ImageLayout)
{
    // OpenGL: implicit synchronization
}

// ---- RenderPass ----

void GLCommandBuffer::BeginRenderPass(rhi::RenderPassHandle /*renderPass*/,
                                       rhi::FramebufferHandle framebuffer,
                                       const rhi::Rect2D& /*renderArea*/,
                                       const rhi::ClearValue* clearValues,
                                       uint32_t clearValueCount)
{
    m_InRenderPass = true;

    // Bind FBO
    if (framebuffer.IsValid())
    {
        // [HACK: 通过 OpenGLDevice 查询 GL FBO handle]
        GLuint fbo = 0;  // TODO: resolve from handle via device
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        m_CurrentFbo = fbo;
    }
    else
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        m_CurrentFbo = 0;
    }

    // Apply clears
    GLbitfield clearMask = 0;
    for (uint32_t i = 0; i < clearValueCount; i++)
    {
        if (clearValues[i].Color.RGBA[3] > 0.0f || clearValues[i].Color.RGBA[0] > 0.0f
            || clearValues[i].Color.RGBA[1] > 0.0f || clearValues[i].Color.RGBA[2] > 0.0f)
        {
            glClearColor(clearValues[i].Color.RGBA[0], clearValues[i].Color.RGBA[1],
                         clearValues[i].Color.RGBA[2], clearValues[i].Color.RGBA[3]);
            clearMask |= GL_COLOR_BUFFER_BIT;
        }
    }
    if (clearMask)
        glClear(clearMask);
}

void GLCommandBuffer::EndRenderPass()
{
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

    // [HACK: resolve GL program from device's pipeline slot]
    // GLuint program = resolve(pipeline);
    // glUseProgram(program);
}

void GLCommandBuffer::BindGeometry(rhi::GeometryHandle geometry)
{
    if (!geometry.IsValid())
        return;

    uint32_t gid = geometry.GetId();
    if (gid == m_CurrentGeometryId)
        return;
    m_CurrentGeometryId = gid;

    // [HACK: lazy VAO creation from Pipeline.vertexLayout + Geometry buffers]
    // This is implemented in Task 14 (Pass refactoring).
}

// ---- Descriptor Binding ----

void GLCommandBuffer::BindUniformBuffer(uint32_t /*slot*/, rhi::BufferHandle /*buffer*/,
                                         uint64_t /*offset*/, uint64_t /*range*/)
{
    // [HACK: glBindBufferRange — Task 14 完整实现]
}

void GLCommandBuffer::BindStorageBuffer(uint32_t /*slot*/, rhi::BufferHandle /*buffer*/,
                                         uint64_t /*offset*/, uint64_t /*range*/)
{
    // [HACK: glBindBufferBase — Task 14 完整实现]
}

void GLCommandBuffer::BindTexture(uint32_t /*slot*/, rhi::TextureHandle /*texture*/,
                                   rhi::SamplerHandle /*sampler*/)
{
    // [HACK: glActiveTexture + glBindTexture + glBindSampler — Task 14 完整实现]
}

void GLCommandBuffer::BindStorageTexture(uint32_t /*slot*/, rhi::TextureHandle /*texture*/)
{
    // [HACK: glBindImageTexture — Task 14 完整实现]
}

// ---- Push Constants ----

void GLCommandBuffer::PushConstants(const void* /*data*/, uint32_t /*size*/, uint32_t /*offset*/)
{
    // [HACK: glUniform* or staging UBO — Task 14 完整实现]
}

// ---- Draw ----

void GLCommandBuffer::Draw(uint32_t vertexCount, uint32_t firstVertex, uint32_t instanceCount)
{
    // [HACK: glDrawArrays — Task 14 完整实现]
    (void)vertexCount; (void)firstVertex; (void)instanceCount;
}

void GLCommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t firstIndex,
                                   int32_t vertexOffset, uint32_t instanceCount)
{
    // [HACK: glDrawElements — Task 14 完整实现]
    (void)indexCount; (void)firstIndex; (void)vertexOffset; (void)instanceCount;
}

// ---- Compute ----

void GLCommandBuffer::DispatchCompute(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    glDispatchCompute(groupX, groupY, groupZ);
}

// ---- Copy ----

void GLCommandBuffer::CopyBuffer(rhi::BufferHandle /*src*/, rhi::BufferHandle /*dst*/,
                                  uint64_t /*srcOffset*/, uint64_t /*dstOffset*/,
                                  uint64_t /*size*/)
{
    // [HACK: glCopyBufferSubData — Task 14 完整实现]
}

void GLCommandBuffer::CopyBufferToTexture(rhi::BufferHandle /*src*/,
                                           rhi::TextureHandle /*dst*/,
                                           const rhi::Offset3D& /*dstOffset*/,
                                           const rhi::Extent3D& /*dstExtent*/)
{
    // [HACK: glTexSubImage2D from PBO — Task 14 完整实现]
}

void GLCommandBuffer::CopyTextureToBuffer(rhi::TextureHandle /*src*/,
                                           rhi::BufferHandle /*dst*/,
                                           const rhi::Offset3D& /*srcOffset*/,
                                           const rhi::Extent3D& /*srcExtent*/)
{
    // [HACK: glGetTexImage into PBO — Task 14 完整实现]
}

// ---- Debug ----

void GLCommandBuffer::BeginDebugLabel(const char* label, std::array<float, 4> /*color*/)
{
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, label);
}

void GLCommandBuffer::EndDebugLabel()
{
    glPopDebugGroup();
}

void GLCommandBuffer::InsertDebugMarker(const char* /*marker*/)
{
    // glDebugMessageInsert not universally available; use labels instead
}

} // namespace Vulkitten
