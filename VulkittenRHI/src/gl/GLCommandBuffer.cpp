#include "GLCommandBuffer.hpp"
#include "GLDevice.hpp"
#include "GLResources.hpp"
#include "rhi/ResourceManager.hpp"

#include <glad/glad.h>

#include <cstring>
#include <cstdio>

namespace rhi {

// ============================================================
// Construction
// ============================================================

GLCommandBuffer::GLCommandBuffer(GLDevice& device)
    : m_Device(device)
    , m_Resources(device.GetResourceManager())
{
}

GLCommandBuffer::~GLCommandBuffer()
{
    // VAOs are owned by GLPipelineResource (persistent across frames)
}

void GLCommandBuffer::Begin()           { m_InRenderPass = false; }
void GLCommandBuffer::End()             { if (m_InRenderPass) EndRenderPass(); }
CommandBufferLevel GLCommandBuffer::GetLevel() const { return CommandBufferLevel::Primary; }

void GLCommandBuffer::Barrier(PipelineStage, AccessFlags, PipelineStage, AccessFlags) {}
void GLCommandBuffer::Barrier(TextureHandle, PipelineStage, AccessFlags,
                               PipelineStage, AccessFlags, ImageLayout, ImageLayout) {}

// ============================================================
// RenderPass
// ============================================================

void GLCommandBuffer::BeginRenderPass(RenderPassHandle renderPass, FramebufferHandle framebuffer,
                                       const Rect2D& /*renderArea*/,
                                       const ClearValue* clearValues, uint32_t clearValueCount)
{
    m_InRenderPass = true;

    // Look up FBO from device cache (created by GLDevice::CreateFramebuffer)
    GLuint fbo = 0;
    if (framebuffer.IsValid())
    {
        fbo = m_Device.GetFbo(framebuffer.GetId());
    }
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    m_CurrentFbo = fbo;

    // Look up render pass desc for clear value count
    auto* rpDesc = m_Resources.GetRenderPassDesc(renderPass.GetId());
    uint32_t numColor = rpDesc ? static_cast<uint32_t>(rpDesc->ColorAttachments.size()) : 1;

    GLbitfield clearMask = 0;
    uint32_t clearCount = std::min(clearValueCount, numColor);
    for (uint32_t i = 0; i < clearCount; ++i)
    {
        glClearColor(clearValues[i].Color.RGBA[0], clearValues[i].Color.RGBA[1],
                     clearValues[i].Color.RGBA[2], clearValues[i].Color.RGBA[3]);
        clearMask |= GL_COLOR_BUFFER_BIT;
    }
    // Always clear depth+stencil on default framebuffer
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

// ============================================================
// BindPipeline — PSO-driven GL state application
// ============================================================

void GLCommandBuffer::BindPipeline(PipelineHandle pipeline)
{
    if (pipeline.GetId() == m_CurrentPipelineId) return;
    m_CurrentPipelineId = pipeline.GetId();

    // Look up pipeline resource from ResourceManager
    auto* pipeRes = dynamic_cast<GLPipelineResource*>(m_Resources.GetPipeline(pipeline));
    m_CurrentPipeline = pipeRes;

    if (!pipeRes) return;

    // Apply ALL GL state from the PSO
    pipeRes->ApplyGLState();
}

// ============================================================
// BindGeometry — lazy VAO via GLPipelineResource
// ============================================================

void GLCommandBuffer::BindGeometry(GeometryHandle geometry)
{
    if (!geometry.IsValid()) return;
    m_CurrentGeometryId = geometry.GetId();

    // Look up geometry resource
    auto* geoRes = dynamic_cast<GLGeometryResource*>(m_Resources.GetGeometry(geometry));
    m_CurrentGeometry = geoRes;

    if (!m_CurrentPipeline || !geoRes) return;

    const auto& geoDesc = geoRes->GetDesc();

    // Collect vertex buffer resources for VAO creation
    std::vector<GLBufferResource*> vtxBuffers;
    vtxBuffers.reserve(geoDesc.VertexBufferCount);
    for (uint32_t i = 0; i < geoDesc.VertexBufferCount; ++i)
    {
        auto* buf = dynamic_cast<GLBufferResource*>(
            m_Resources.GetBuffer(geoDesc.VertexBuffers[i]));
        vtxBuffers.push_back(buf);
    }

    GLBufferResource* idxBuf = nullptr;
    if (geoDesc.IndexBuffer.IsValid())
    {
        idxBuf = dynamic_cast<GLBufferResource*>(
            m_Resources.GetBuffer(geoDesc.IndexBuffer));
    }

    // Get or create VAO (lazy, cached per pipeline+geometry)
    GLuint vao = m_CurrentPipeline->GetOrCreateVAO(
        geometry.GetId(), vtxBuffers, idxBuf, geoDesc.IndexType);

    glBindVertexArray(vao);
}

// ============================================================
// BindUniformBuffer
// ============================================================

void GLCommandBuffer::BindUniformBuffer(uint32_t slot, BufferHandle buffer,
                                         uint64_t offset, uint64_t range)
{
    auto* bufRes = dynamic_cast<GLBufferResource*>(m_Resources.GetBuffer(buffer));
    if (!bufRes || !bufRes->GetGLBuffer()) return;

    GLsizeiptr size = (range > 0) ? static_cast<GLsizeiptr>(range)
                                   : static_cast<GLsizeiptr>(bufRes->GetSize() - offset);
    glBindBufferRange(GL_UNIFORM_BUFFER, slot, bufRes->GetGLBuffer(),
                      static_cast<GLintptr>(offset), size);
}

void GLCommandBuffer::BindStorageBuffer(uint32_t slot, BufferHandle buffer,
                                         uint64_t offset, uint64_t range)
{
    auto* bufRes = dynamic_cast<GLBufferResource*>(m_Resources.GetBuffer(buffer));
    if (!bufRes || !bufRes->GetGLBuffer()) return;

    GLsizeiptr size = (range > 0) ? static_cast<GLsizeiptr>(range)
                                   : static_cast<GLsizeiptr>(bufRes->GetSize() - offset);
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, slot, bufRes->GetGLBuffer(),
                      static_cast<GLintptr>(offset), size);
}

// ============================================================
// Texture / PushConstants (stubs)
// ============================================================

void GLCommandBuffer::BindTexture(uint32_t, TextureHandle, SamplerHandle) {}
void GLCommandBuffer::BindStorageTexture(uint32_t, TextureHandle) {}
void GLCommandBuffer::PushConstants(const void*, uint32_t, uint32_t) {}

// ============================================================
// Draw
// ============================================================

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
    const auto* geoDesc = m_CurrentGeometry ? &m_CurrentGeometry->GetDesc() : nullptr;
    GLenum idxType = (geoDesc && geoDesc->IndexType == IndexType::UInt16) ?
        GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
    const void* idxOff = reinterpret_cast<const void*>(
        static_cast<uintptr_t>(firstIndex * (idxType == GL_UNSIGNED_SHORT ? 2 : 4)));

    (void)vertexOffset; // [HACK: vertexOffset not supported via glDrawElements]
    if (instanceCount > 1)
        glDrawElementsInstanced(GL_TRIANGLES, indexCount, idxType, idxOff, instanceCount);
    else
        glDrawElements(GL_TRIANGLES, indexCount, idxType, idxOff);
}

// ============================================================
// Compute (stub)
// ============================================================
void GLCommandBuffer::DispatchCompute(uint32_t, uint32_t, uint32_t) {}

// ============================================================
// Copy (stubs)
// ============================================================
void GLCommandBuffer::CopyBuffer(BufferHandle, BufferHandle, uint64_t, uint64_t, uint64_t) {}
void GLCommandBuffer::CopyBufferToTexture(BufferHandle, TextureHandle,
                                           const Offset3D&, const Extent3D&) {}
void GLCommandBuffer::CopyTextureToBuffer(TextureHandle, BufferHandle,
                                           const Offset3D&, const Extent3D&) {}

// ============================================================
// Debug (stubs)
// ============================================================
void GLCommandBuffer::BeginDebugLabel(const char*, std::array<float, 4>) {}
void GLCommandBuffer::EndDebugLabel() {}
void GLCommandBuffer::InsertDebugMarker(const char*) {}

} // namespace rhi
