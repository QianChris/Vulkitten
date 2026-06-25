#include "GLCommandBuffer.hpp"
#include "GLDevice.hpp"

#include <glad/glad.h>

#include <cstring>
#include <cstdio>

namespace rhi {

GLCommandBuffer::GLCommandBuffer(GLDevice& device)
    : m_Device(device)
{
}

GLCommandBuffer::~GLCommandBuffer()
{
    // VAOs are owned by GLDevice (persistent across frames)
}

void GLCommandBuffer::Begin()           { m_InRenderPass = false; }
void GLCommandBuffer::End()             { if (m_InRenderPass) EndRenderPass(); }
CommandBufferLevel GLCommandBuffer::GetLevel() const { return CommandBufferLevel::Primary; }

void GLCommandBuffer::Barrier(PipelineStage, AccessFlags, PipelineStage, AccessFlags) {}
void GLCommandBuffer::Barrier(TextureHandle, PipelineStage, AccessFlags,
                               PipelineStage, AccessFlags, ImageLayout, ImageLayout) {}

// ---- RenderPass ----

void GLCommandBuffer::BeginRenderPass(RenderPassHandle renderPass, FramebufferHandle framebuffer,
                                       const Rect2D& /*renderArea*/,
                                       const ClearValue* clearValues, uint32_t clearValueCount)
{
    m_InRenderPass = true;

    GLuint fbo = 0;
    if (framebuffer.IsValid())
    {
        auto* slot = m_Device.GetSlot(framebuffer.GetId());
        if (slot && slot->Alive) fbo = static_cast<GLuint>(slot->GpuHandle);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    m_CurrentFbo = fbo;

    auto* rpDesc = m_Device.GetRenderPassDesc(renderPass.GetId());
    uint32_t numColor = rpDesc ? static_cast<uint32_t>(rpDesc->ColorAttachments.size()) : 1;
    bool hasDepth = rpDesc && rpDesc->DepthStencilAttachment.Format != Format::Unknown;

    GLbitfield clearMask = 0;
    for (uint32_t i = 0; i < clearValueCount && i < numColor; ++i)
    {
        glClearColor(clearValues[i].Color.RGBA[0], clearValues[i].Color.RGBA[1],
                     clearValues[i].Color.RGBA[2], clearValues[i].Color.RGBA[3]);
        clearMask |= GL_COLOR_BUFFER_BIT;
    }
    // Always clear depth+stencil on default framebuffer (GLFW creates one by default).
    // Without this, uninitialized depth values cause fragments to fail depth test.
    glClearDepthf(1.0f);
    clearMask |= GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
    if (clearMask)
        glClear(clearMask);
}

void GLCommandBuffer::EndRenderPass()
{
    m_InRenderPass = false;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    m_CurrentFbo = 0;
}

// ---- BindPipeline ----

void GLCommandBuffer::BindPipeline(PipelineHandle pipeline)
{
    if (pipeline.GetId() == m_CurrentPipelineId) return;
    m_CurrentPipelineId = pipeline.GetId();

    auto* meta = m_Device.GetPipelineMeta(pipeline.GetId());
    if (!meta) return;
    glUseProgram(meta->GlProgram);
}

// ---- BindGeometry (lazy VAO via GLDevice) ----

void GLCommandBuffer::BindGeometry(GeometryHandle geometry)
{
    if (!geometry.IsValid()) return;
    m_CurrentGeometryId = geometry.GetId();

    uint64_t vaoKey = (static_cast<uint64_t>(m_CurrentPipelineId) << 32) | geometry.GetId();
    uint32_t vao = m_Device.GetOrCreateVAO(vaoKey, m_CurrentPipelineId, geometry.GetId());
    glBindVertexArray(vao);
}

// ---- BindUniformBuffer ----

void GLCommandBuffer::BindUniformBuffer(uint32_t slot, BufferHandle buffer,
                                         uint64_t offset, uint64_t range)
{
    auto* bufMeta = m_Device.GetBufferMeta(buffer.GetId());
    if (!bufMeta) return;
    GLsizeiptr size = (range > 0) ? static_cast<GLsizeiptr>(range)
                                   : static_cast<GLsizeiptr>(bufMeta->Size - offset);
    glBindBufferRange(GL_UNIFORM_BUFFER, slot, bufMeta->GlBuffer,
                      static_cast<GLintptr>(offset), size);
}

void GLCommandBuffer::BindStorageBuffer(uint32_t slot, BufferHandle buffer,
                                         uint64_t offset, uint64_t range)
{
    auto* bufMeta = m_Device.GetBufferMeta(buffer.GetId());
    if (!bufMeta) return;
    GLsizeiptr size = (range > 0) ? static_cast<GLsizeiptr>(range)
                                   : static_cast<GLsizeiptr>(bufMeta->Size - offset);
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, slot, bufMeta->GlBuffer,
                      static_cast<GLintptr>(offset), size);
}

// ---- Texture / PushConstants (stubs) ----

void GLCommandBuffer::BindTexture(uint32_t, TextureHandle, SamplerHandle) {}
void GLCommandBuffer::BindStorageTexture(uint32_t, TextureHandle) {}
void GLCommandBuffer::PushConstants(const void*, uint32_t, uint32_t) {}

// ---- Draw ----

void GLCommandBuffer::Draw(uint32_t vertexCount, uint32_t firstVertex, uint32_t instanceCount)
{
    if (instanceCount > 1)
        glDrawArraysInstanced(GL_TRIANGLES, firstVertex, vertexCount, instanceCount);
    else
        glDrawArrays(GL_TRIANGLES, firstVertex, vertexCount);
}

void GLCommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t firstIndex,
                                   int32_t vertexOffset, uint32_t instanceCount)
{
    auto* geoDesc = m_Device.GetGeometryDesc(m_CurrentGeometryId);
    GLenum idxType = (geoDesc && geoDesc->IndexType == IndexType::UInt16) ?
        GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
    const void* idxOff = reinterpret_cast<const void*>(
        static_cast<uintptr_t>(firstIndex * (idxType == GL_UNSIGNED_SHORT ? 2 : 4)));
    if (instanceCount > 1)
        glDrawElementsInstanced(GL_TRIANGLES, indexCount, idxType, idxOff, instanceCount);
    else
        glDrawElements(GL_TRIANGLES, indexCount, idxType, idxOff);
}

// ---- Compute (stub) ----
void GLCommandBuffer::DispatchCompute(uint32_t, uint32_t, uint32_t) {}

// ---- Copy (stubs) ----
void GLCommandBuffer::CopyBuffer(BufferHandle, BufferHandle, uint64_t, uint64_t, uint64_t) {}
void GLCommandBuffer::CopyBufferToTexture(BufferHandle, TextureHandle,
                                           const Offset3D&, const Extent3D&) {}
void GLCommandBuffer::CopyTextureToBuffer(TextureHandle, BufferHandle,
                                           const Offset3D&, const Extent3D&) {}

// ---- Debug (stubs) ----
void GLCommandBuffer::BeginDebugLabel(const char*, std::array<float, 4>) {}
void GLCommandBuffer::EndDebugLabel() {}
void GLCommandBuffer::InsertDebugMarker(const char*) {}

} // namespace rhi
