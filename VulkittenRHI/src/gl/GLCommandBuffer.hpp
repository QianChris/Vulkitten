#pragma once

#include "rhi/ICommandBuffer.hpp"
#include "rhi/Core/Handle.hpp"
#include "rhi/ResourceDescs.hpp"

#include <cstdint>

namespace rhi {

class GLDevice;

// ============================================================
// GLCommandBuffer — OpenGL ICommandBuffer implementation.
//
// OpenGL is immediate-mode; most commands execute directly.
// State caching avoids redundant GL calls.
//
// Lazy VAO: BindGeometry creates a VAO on first use from
// the Pipeline's vertexLayout + Geometry's buffer handles.
// ============================================================

class GLCommandBuffer : public ICommandBuffer
{
public:
    explicit GLCommandBuffer(GLDevice& device);
    ~GLCommandBuffer() override;

    // ---- ICommandBuffer ----
    void Begin() override;
    void End() override;

    CommandBufferLevel GetLevel() const override;

    void Barrier(PipelineStage srcStage, AccessFlags srcAccess,
                 PipelineStage dstStage, AccessFlags dstAccess) override;
    void Barrier(TextureHandle texture,
                 PipelineStage srcStage, AccessFlags srcAccess,
                 PipelineStage dstStage, AccessFlags dstAccess,
                 ImageLayout oldLayout, ImageLayout newLayout) override;

    void BeginRenderPass(RenderPassHandle renderPass, FramebufferHandle framebuffer,
                         const Rect2D& renderArea,
                         const ClearValue* clearValues, uint32_t clearValueCount) override;
    void EndRenderPass() override;

    void BindPipeline(PipelineHandle pipeline) override;
    void BindGeometry(GeometryHandle geometry) override;

    void BindUniformBuffer(uint32_t slot, BufferHandle buffer,
                           uint64_t offset, uint64_t range) override;
    void BindStorageBuffer(uint32_t slot, BufferHandle buffer,
                           uint64_t offset, uint64_t range) override;
    void BindTexture(uint32_t slot, TextureHandle texture,
                     SamplerHandle sampler) override;
    void BindStorageTexture(uint32_t slot, TextureHandle texture) override;

    void PushConstants(const void* data, uint32_t size, uint32_t offset = 0) override;

    void Draw(uint32_t vertexCount, uint32_t firstVertex = 0,
              uint32_t instanceCount = 1) override;
    void DrawIndexed(uint32_t indexCount, uint32_t firstIndex = 0,
                     int32_t vertexOffset = 0, uint32_t instanceCount = 1) override;

    void DispatchCompute(uint32_t groupX, uint32_t groupY, uint32_t groupZ = 1) override;

    void CopyBuffer(BufferHandle src, BufferHandle dst,
                    uint64_t srcOffset, uint64_t dstOffset, uint64_t size) override;
    void CopyBufferToTexture(BufferHandle src, TextureHandle dst,
                             const Offset3D& dstOffset, const Extent3D& dstExtent) override;
    void CopyTextureToBuffer(TextureHandle src, BufferHandle dst,
                             const Offset3D& srcOffset, const Extent3D& srcExtent) override;

    void BeginDebugLabel(const char* label,
                         std::array<float, 4> color = {1,1,1,1}) override;
    void EndDebugLabel() override;
    void InsertDebugMarker(const char* marker) override;

private:
    GLDevice& m_Device;

    // State cache
    uint32_t m_CurrentPipelineId = 0;
    uint32_t m_CurrentGeometryId = 0;
    uint32_t m_CurrentFbo = 0;
    bool     m_InRenderPass = false;
};

} // namespace rhi
