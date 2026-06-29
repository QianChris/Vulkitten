#pragma once

#include "rhi/Core/Export.hpp"
#include "rhi/ICommandBuffer.hpp"
#include "rhi/Core/Handle.hpp"

#include <cstdint>
#include <unordered_map>

namespace rhi {

class VKDevice;
class ResourceManager;
class VKPipelineResource;
class VKBufferResource;
class VKGeometryResource;

class RHI_API VKCommandBuffer : public ICommandBuffer
{
public:
    explicit VKCommandBuffer(VKDevice& device, void* vkCommandBuffer, uint32_t frameIndex = 0);
    ~VKCommandBuffer() override;

    static void ClearDescriptorSetCache();

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
    void DrawIndirect(BufferHandle indirectBuffer, uint64_t offset,
                      uint32_t drawCount, uint32_t stride) override;

    void DispatchCompute(uint32_t groupX, uint32_t groupY, uint32_t groupZ = 1) override;
    void DispatchIndirect(BufferHandle indirectBuffer, uint64_t offset) override;

    void CopyBuffer(BufferHandle src, BufferHandle dst,
                    uint64_t srcOffset, uint64_t dstOffset, uint64_t size) override;
    void CopyBufferToTexture(BufferHandle src, TextureHandle dst,
                             const Offset3D& dstOffset, const Extent3D& dstExtent) override;
    void CopyTextureToBuffer(TextureHandle src, BufferHandle dst,
                             const Offset3D& srcOffset, const Extent3D& srcExtent) override;

    void ResetQueryPool(QueryPoolHandle pool, uint32_t firstQuery, uint32_t queryCount) override;
    void BeginQuery(QueryPoolHandle pool, uint32_t queryIndex) override;
    void EndQuery(QueryPoolHandle pool, uint32_t queryIndex) override;
    void WriteTimestamp(PipelineStage stage, QueryPoolHandle pool, uint32_t queryIndex) override;

    void BeginDebugLabel(const char* label,
                         std::array<float, 4> color = {1,1,1,1}) override;
    void EndDebugLabel() override;
    void InsertDebugMarker(const char* marker) override;

    void* GetVkCommandBuffer() const { return m_VkCmd; }

private:
    void WriteDescriptorSet(uint32_t slot, uint32_t type,
                            void* vkBuf, uint64_t offset, uint64_t range);

    VKDevice&       m_Device;
    ResourceManager& m_Resources;
    void*           m_VkCmd = nullptr;
    bool            m_InRenderPass = false;
    bool            m_IsRecording = false;
    uint32_t        m_CurrentPipelineId = 0;
    uint32_t        m_CurrentGeometryId = 0;
    uint32_t        m_FrameIndex = 0;

    VKPipelineResource* m_CurrentPipeline = nullptr;
    VKGeometryResource* m_CurrentGeometry = nullptr;

    // Per-frame descriptor set cache pointer (points to VKDevice's cache)
    std::unordered_map<uint64_t, void*>* m_DescriptorCache = nullptr;
};

} // namespace rhi
