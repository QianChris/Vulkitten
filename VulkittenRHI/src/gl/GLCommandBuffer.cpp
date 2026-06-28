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

GLCommandBuffer::~GLCommandBuffer() = default;

void GLCommandBuffer::Begin()           { m_InRenderPass = false; }
void GLCommandBuffer::End()             { if (m_InRenderPass) EndRenderPass(); }
CommandBufferLevel GLCommandBuffer::GetLevel() const { return CommandBufferLevel::Primary; }

// ---- Barriers (GL: mostly no-op; texture barrier for feedback loops) ----
void GLCommandBuffer::Barrier(PipelineStage, AccessFlags, PipelineStage, AccessFlags) {}
void GLCommandBuffer::Barrier(TextureHandle, PipelineStage, AccessFlags,
                               PipelineStage, AccessFlags, ImageLayout, ImageLayout) {}

// ============================================================
// RenderPass
// ============================================================

void GLCommandBuffer::BeginRenderPass(RenderPassHandle renderPass, FramebufferHandle framebuffer,
                                       const Rect2D& renderArea,
                                       const ClearValue* clearValues, uint32_t clearValueCount)
{
    m_InRenderPass = true;

    GLuint fbo = 0;
    if (framebuffer.IsValid())
        fbo = m_Device.GetFbo(framebuffer.GetId());

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    m_CurrentFbo = fbo;

    // Apply scissor from render area if pipeline has scissor enabled
    if (m_CurrentPipeline && m_CurrentPipeline->GetRasterState().ScissorEnable)
    {
        glEnable(GL_SCISSOR_TEST);
        glScissor(renderArea.Offset.X, renderArea.Offset.Y,
                  renderArea.Extent.Width, renderArea.Extent.Height);
    }

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
    glClearDepthf(1.0f);
    clearMask |= GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
    if (clearMask)
        glClear(clearMask);
}

void GLCommandBuffer::EndRenderPass()
{
    m_InRenderPass = false;
    if (m_CurrentPipeline && m_CurrentPipeline->GetRasterState().ScissorEnable)
        glDisable(GL_SCISSOR_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    m_CurrentFbo = 0;
}

// ============================================================
// BindPipeline - PSO-driven GL state
// ============================================================

void GLCommandBuffer::BindPipeline(PipelineHandle pipeline)
{
    if (pipeline.GetId() == m_CurrentPipelineId) return;
    m_CurrentPipelineId = pipeline.GetId();

    m_CurrentPipeline = dynamic_cast<GLPipelineResource*>(m_Resources.GetPipeline(pipeline));
    if (!m_CurrentPipeline) return;

    m_CurrentPipeline->ApplyGLState();
}

// ============================================================
// BindGeometry - lazy VAO
// ============================================================

void GLCommandBuffer::BindGeometry(GeometryHandle geometry)
{
    if (!geometry.IsValid()) return;
    m_CurrentGeometryId = geometry.GetId();

    m_CurrentGeometry = dynamic_cast<GLGeometryResource*>(m_Resources.GetGeometry(geometry));
    if (!m_CurrentPipeline || !m_CurrentGeometry) return;

    const auto& geoDesc = m_CurrentGeometry->GetDesc();

    std::vector<GLBufferResource*> vtxBuffers;
    vtxBuffers.reserve(geoDesc.VertexBufferCount);
    for (uint32_t i = 0; i < geoDesc.VertexBufferCount; ++i)
        vtxBuffers.push_back(dynamic_cast<GLBufferResource*>(
            m_Resources.GetBuffer(geoDesc.VertexBuffers[i])));

    GLBufferResource* idxBuf = nullptr;
    if (geoDesc.IndexBuffer.IsValid())
        idxBuf = dynamic_cast<GLBufferResource*>(
            m_Resources.GetBuffer(geoDesc.IndexBuffer));

    GLuint vao = m_CurrentPipeline->GetOrCreateVAO(
        geometry.GetId(), vtxBuffers, idxBuf, geoDesc.IndexType);
    glBindVertexArray(vao);
}

// ============================================================
// Buffer Binding
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
// Texture Binding
// ============================================================

void GLCommandBuffer::BindTexture(uint32_t slot, TextureHandle texture, SamplerHandle sampler)
{
    auto* texRes = dynamic_cast<GLTextureResource*>(m_Resources.GetTexture(texture));
    if (!texRes || !texRes->GetGLTexture()) return;

    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(texRes->GetGLTarget(), texRes->GetGLTexture());

    // Bind sampler if provided
    if (sampler.IsValid())
    {
        auto* smpRes = dynamic_cast<GLSamplerResource*>(m_Resources.GetSampler(sampler));
        if (smpRes && smpRes->GetGLSampler())
            glBindSampler(slot, smpRes->GetGLSampler());
    }
}

void GLCommandBuffer::BindStorageTexture(uint32_t slot, TextureHandle texture)
{
    auto* texRes = dynamic_cast<GLTextureResource*>(m_Resources.GetTexture(texture));
    if (!texRes || !texRes->GetGLTexture()) return;

    // Bind as image for compute shader read/write
    glBindImageTexture(slot, texRes->GetGLTexture(), 0, GL_FALSE, 0,
                       GL_READ_WRITE, FormatToGLInternal(texRes->GetFormat()));
}

// ============================================================
// Push Constants
// Uses glProgramUniform* (GL 4.1+ separate shader objects)
// ============================================================

void GLCommandBuffer::PushConstants(const void* data, uint32_t size, uint32_t offset)
{
    if (!m_CurrentPipeline) return;

    GLuint program = m_CurrentPipeline->GetGLProgram();
    if (!program) return;

    // Push constants map to a uniform block at binding 0 with std140 layout.
    // For simplicity, treat as raw bytes via glProgramUniform*.
    // [HACK: uniform location lookup omitted - assumes push constant block
    //  uses explicit layout(binding=0, std140) uniform PushConstants { ... };]
    // For MVP: push constant data is written via glProgramUniform1fv etc.
    // Real implementation would use reflection or convention-based location mapping.

    (void)offset;
    (void)size;
    // [HACK: full push constant implementation requires shader reflection]
}

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

    if (instanceCount > 1)
    {
        if (vertexOffset != 0)
            glDrawElementsInstancedBaseVertex(GL_TRIANGLES, indexCount, idxType,
                                              idxOff, instanceCount, vertexOffset);
        else
            glDrawElementsInstanced(GL_TRIANGLES, indexCount, idxType, idxOff, instanceCount);
    }
    else
    {
        if (vertexOffset != 0)
            glDrawElementsBaseVertex(GL_TRIANGLES, indexCount, idxType,
                                     idxOff, vertexOffset);
        else
            glDrawElements(GL_TRIANGLES, indexCount, idxType, idxOff);
    }
}

void GLCommandBuffer::DrawIndirect(BufferHandle indirectBuffer, uint64_t offset,
                                    uint32_t drawCount, uint32_t stride)
{
    auto* bufRes = dynamic_cast<GLBufferResource*>(m_Resources.GetBuffer(indirectBuffer));
    if (!bufRes || !bufRes->GetGLBuffer()) return;

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, bufRes->GetGLBuffer());
    glMultiDrawArraysIndirect(GL_TRIANGLES, reinterpret_cast<const void*>(static_cast<uintptr_t>(offset)),
                              static_cast<GLsizei>(drawCount), static_cast<GLsizei>(stride));
}

// ============================================================
// Compute
// ============================================================

void GLCommandBuffer::DispatchCompute(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    glDispatchCompute(groupX, groupY, groupZ);
}

void GLCommandBuffer::DispatchIndirect(BufferHandle indirectBuffer, uint64_t offset)
{
    auto* bufRes = dynamic_cast<GLBufferResource*>(m_Resources.GetBuffer(indirectBuffer));
    if (!bufRes || !bufRes->GetGLBuffer()) return;

    glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, bufRes->GetGLBuffer());
    glDispatchComputeIndirect(static_cast<GLintptr>(offset));
}

// ============================================================
// Copy (stubs - require texture implementation)
// ============================================================

void GLCommandBuffer::CopyBuffer(BufferHandle, BufferHandle, uint64_t, uint64_t, uint64_t) {}
void GLCommandBuffer::CopyBufferToTexture(BufferHandle, TextureHandle,
                                           const Offset3D&, const Extent3D&) {}
void GLCommandBuffer::CopyTextureToBuffer(TextureHandle, BufferHandle,
                                           const Offset3D&, const Extent3D&) {}

// ============================================================
// GPU Queries / Timestamps
// ============================================================

void GLCommandBuffer::ResetQueryPool(QueryPoolHandle /*pool*/, uint32_t /*firstQuery*/,
                                      uint32_t /*queryCount*/)
{
    // GL queries are implicitly "reset" when begun; no explicit reset needed
}

void GLCommandBuffer::BeginQuery(QueryPoolHandle pool, uint32_t queryIndex)
{
    GLuint queryId = m_Device.GetQueryGL(pool.GetId(), queryIndex);
    if (queryId)
        glBeginQuery(GL_TIME_ELAPSED, queryId);
}

void GLCommandBuffer::EndQuery(QueryPoolHandle pool, uint32_t queryIndex)
{
    GLuint queryId = m_Device.GetQueryGL(pool.GetId(), queryIndex);
    if (queryId)
        glEndQuery(GL_TIME_ELAPSED);
}

void GLCommandBuffer::WriteTimestamp(PipelineStage /*stage*/, QueryPoolHandle pool, uint32_t queryIndex)
{
    GLuint queryId = m_Device.GetQueryGL(pool.GetId(), queryIndex);
    if (queryId)
        glQueryCounter(queryId, GL_TIMESTAMP);
}

// ============================================================
// Debug Markers (GL_KHR_debug / GL 4.3+)
// ============================================================

void GLCommandBuffer::BeginDebugLabel(const char* label, std::array<float, 4> color)
{
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, label);
    (void)color;
}

void GLCommandBuffer::EndDebugLabel()
{
    glPopDebugGroup();
}

void GLCommandBuffer::InsertDebugMarker(const char* marker)
{
    glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER,
                         0, GL_DEBUG_SEVERITY_NOTIFICATION, -1, marker);
}

} // namespace rhi
