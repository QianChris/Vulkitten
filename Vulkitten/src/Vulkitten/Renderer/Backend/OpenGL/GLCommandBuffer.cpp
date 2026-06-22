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

    auto* slot = m_Device.GetSlot(pid);
    if (!slot || !slot->Alive)
    {
        VKT_CORE_WARN("GLCommandBuffer::BindPipeline: invalid handle id={0}", pid);
        return;
    }

    GLuint program = static_cast<GLuint>(slot->GpuHandle);
    if (program)
        glUseProgram(program);
}

void GLCommandBuffer::BindGeometry(rhi::GeometryHandle geometry)
{
    if (!geometry.IsValid())
        return;

    uint32_t gid = geometry.GetId();
    if (gid == m_CurrentGeometryId && m_CurrentPipelineId != 0)
        return;  // Same geo + pipeline = VAO already bound
    m_CurrentGeometryId = gid;

    if (m_CurrentPipelineId == 0)
        return;  // Can't create VAO without pipeline vertex layout

    // VAO cache lookup
    uint64_t vaoKey = (static_cast<uint64_t>(m_CurrentPipelineId) << 32) | gid;
    auto vaoIt = m_VaoCache.find(vaoKey);
    if (vaoIt != m_VaoCache.end())
    {
        glBindVertexArray(static_cast<GLuint>(vaoIt->second));
        return;
    }

    // Lazy VAO creation
    const auto* layout = m_Device.GetPipelineVertexLayout(m_CurrentPipelineId);
    const auto* geoDesc = m_Device.GetGeometryDesc(gid);
    if (!layout || !geoDesc)
        return;

    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Bind index buffer if present
    if (geoDesc->IndexBuffer.IsValid())
    {
        auto* ibSlot = m_Device.GetSlot(geoDesc->IndexBuffer.GetId());
        if (ibSlot && ibSlot->GpuHandle)
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLuint>(ibSlot->GpuHandle));
    }

    // Bind vertex buffers and set attributes
    for (const auto& attr : *layout)
    {
        if (attr.BufferSlot >= geoDesc->VertexBufferCount)
            continue;

        auto& vbHandle = geoDesc->VertexBuffers[attr.BufferSlot];
        if (!vbHandle.IsValid())
            continue;

        auto* vbSlot = m_Device.GetSlot(vbHandle.GetId());
        if (!vbSlot || !vbSlot->GpuHandle)
            continue;

        GLuint glVb = static_cast<GLuint>(vbSlot->GpuHandle);
        glBindBuffer(GL_ARRAY_BUFFER, glVb);

        // Map rhi::Format to GL type/count
        GLint  compCount = 0;
        GLenum glType   = GL_FLOAT;
        bool   normalized = false;
        switch (attr.Format)
        {
            case rhi::Format::R32_FLOAT:       compCount = 1; glType = GL_FLOAT; break;
            case rhi::Format::RG32_FLOAT:      compCount = 2; glType = GL_FLOAT; break;
            case rhi::Format::RGB32_FLOAT:     compCount = 3; glType = GL_FLOAT; break;
            case rhi::Format::RGBA32_FLOAT:    compCount = 4; glType = GL_FLOAT; break;
            case rhi::Format::R32_UINT:        compCount = 1; glType = GL_UNSIGNED_INT; break;
            case rhi::Format::R32_SINT:        compCount = 1; glType = GL_INT; break;
            case rhi::Format::RG32_UINT:       compCount = 2; glType = GL_UNSIGNED_INT; break;
            case rhi::Format::RGBA32_UINT:     compCount = 4; glType = GL_UNSIGNED_INT; break;
            case rhi::Format::RGBA8_UNORM:     compCount = 4; glType = GL_UNSIGNED_BYTE; normalized = true; break;
            default: compCount = static_cast<GLint>(rhi::FormatComponentCount(attr.Format)); break;
        }

        glEnableVertexAttribArray(attr.Location);
        if (glType == GL_FLOAT && !normalized)
            glVertexAttribPointer(attr.Location, compCount, glType, GL_FALSE,
                                  static_cast<GLsizei>(attr.Stride),
                                  reinterpret_cast<const void*>(static_cast<uintptr_t>(attr.Offset)));
        else
            glVertexAttribPointer(attr.Location, compCount, glType, normalized ? GL_TRUE : GL_FALSE,
                                  static_cast<GLsizei>(attr.Stride),
                                  reinterpret_cast<const void*>(static_cast<uintptr_t>(attr.Offset)));
    }

    glBindVertexArray(0); // Unbind to avoid accidental modification

    // Cache the VAO (GLuint fits in uint32_t on all platforms)
    m_VaoCache[vaoKey] = static_cast<uint32_t>(vao);

    // Bind the newly created VAO
    glBindVertexArray(vao);
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
