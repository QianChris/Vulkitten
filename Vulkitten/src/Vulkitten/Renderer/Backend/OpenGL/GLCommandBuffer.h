#pragma once

#include "Vulkitten/RHI/ICommandBuffer.hpp"
#include "Vulkitten/RHI/Handle.hpp"

#include <cstdint>
#include <unordered_map>

namespace Vulkitten {

class OpenGLDevice;

// ============================================================
// GLCommandBuffer - OpenGL ICommandBuffer implementation.
//
// OpenGL is immediate-mode; most commands execute directly.
// State caching avoids redundant GL calls (e.g., skipping
// glUseProgram if the same program is already bound).
//
// Lazy VAO: bindGeometry creates a VAO on first use from
// the Pipeline's vertexLayout + Geometry's buffer handles.
// ============================================================
class GLCommandBuffer : public rhi::ICommandBuffer
{
public:
    explicit GLCommandBuffer(OpenGLDevice& device);
    ~GLCommandBuffer() override;

    // ---- ICommandBuffer ----
    void Begin() override;
    void End() override;

    void Barrier(rhi::PipelineStage srcStage, rhi::AccessFlags srcAccess,
                 rhi::PipelineStage dstStage, rhi::AccessFlags dstAccess) override;
    void Barrier(rhi::TextureHandle texture,
                 rhi::PipelineStage srcStage, rhi::AccessFlags srcAccess,
                 rhi::PipelineStage dstStage, rhi::AccessFlags dstAccess,
                 rhi::ImageLayout oldLayout, rhi::ImageLayout newLayout) override;

    void BeginRenderPass(rhi::RenderPassHandle renderPass, rhi::FramebufferHandle framebuffer,
                         const rhi::Rect2D& renderArea,
                         const rhi::ClearValue* clearValues, uint32_t clearValueCount) override;
    void EndRenderPass() override;

    void BindPipeline(rhi::PipelineHandle pipeline) override;
    void BindGeometry(rhi::GeometryHandle geometry) override;

    void BindUniformBuffer(uint32_t slot, rhi::BufferHandle buffer,
                           uint64_t offset, uint64_t range) override;
    void BindStorageBuffer(uint32_t slot, rhi::BufferHandle buffer,
                           uint64_t offset, uint64_t range) override;
    void BindTexture(uint32_t slot, rhi::TextureHandle texture,
                     rhi::SamplerHandle sampler) override;
    void BindStorageTexture(uint32_t slot, rhi::TextureHandle texture) override;

    void PushConstants(const void* data, uint32_t size, uint32_t offset = 0) override;

    void Draw(uint32_t vertexCount, uint32_t firstVertex = 0,
              uint32_t instanceCount = 1) override;
    void DrawIndexed(uint32_t indexCount, uint32_t firstIndex = 0,
                     int32_t vertexOffset = 0, uint32_t instanceCount = 1) override;

    void DispatchCompute(uint32_t groupX, uint32_t groupY, uint32_t groupZ = 1) override;

    void CopyBuffer(rhi::BufferHandle src, rhi::BufferHandle dst,
                    uint64_t srcOffset, uint64_t dstOffset, uint64_t size) override;
    void CopyBufferToTexture(rhi::BufferHandle src, rhi::TextureHandle dst,
                             const rhi::Offset3D& dstOffset, const rhi::Extent3D& dstExtent) override;
    void CopyTextureToBuffer(rhi::TextureHandle src, rhi::BufferHandle dst,
                             const rhi::Offset3D& srcOffset, const rhi::Extent3D& srcExtent) override;

    void BeginDebugLabel(const char* label,
                         std::array<float, 4> color = {1,1,1,1}) override;
    void EndDebugLabel() override;
    void InsertDebugMarker(const char* marker) override;

private:
    OpenGLDevice& m_Device;

    // State cache
    uint32_t m_CurrentPipelineId = 0;
    uint32_t m_CurrentGeometryId = 0;
    uint32_t m_CurrentFbo = 0;
    bool     m_InRenderPass = false;

    // VAO cache: key = (pipelineId << 32) | geometryId, value = unsigned int VAO
    std::unordered_map<uint64_t, uint32_t> m_VaoCache;
};

} // namespace Vulkitten
