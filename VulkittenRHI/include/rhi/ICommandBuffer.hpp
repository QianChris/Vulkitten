#pragma once

#include "rhi/Core/Handle.hpp"
#include "rhi/Core/Types.hpp"
#include "rhi/Core/Format.hpp"
#include "rhi/ResourceDescs.hpp"

#include <cstdint>
#include <array>

namespace rhi {

// ============================================================
// ICommandBuffer - command recording interface
//
// Recording order:
//   begin() → [barrier(s)] → beginRenderPass() → bind* →
//   draw/dispatch/copy → endRenderPass() → end()
//
// Vulkan:  true recording into VkCommandBuffer.
// OpenGL:  immediate execution with state-change caching.
// ============================================================

class ICommandBuffer
{
public:
    virtual ~ICommandBuffer() = default;

    // ---- Lifecycle ----
    virtual void Begin() = 0;
    virtual void End() = 0;

    // ---- Level (for multi-threaded recording) ----
    virtual CommandBufferLevel GetLevel() const = 0;

    // ---- Barriers ----
    virtual void Barrier(
        PipelineStage SrcStage, AccessFlags SrcAccess,
        PipelineStage DstStage, AccessFlags DstAccess) = 0;

    virtual void Barrier(
        TextureHandle Texture,
        PipelineStage SrcStage, AccessFlags SrcAccess,
        PipelineStage DstStage, AccessFlags DstAccess,
        ImageLayout OldLayout, ImageLayout NewLayout) = 0;

    // ---- RenderPass ----
    virtual void BeginRenderPass(
        RenderPassHandle  RenderPass,
        FramebufferHandle Framebuffer,
        const Rect2D&     RenderArea,
        const ClearValue* ClearValues,
        uint32_t          ClearValueCount) = 0;

    virtual void EndRenderPass() = 0;

    // ---- Pipeline & Geometry Binding ----
    virtual void BindPipeline(PipelineHandle Pipeline) = 0;
    virtual void BindGeometry(GeometryHandle Geometry) = 0;

    // ---- Descriptor Binding (slot-based) ----
    virtual void BindUniformBuffer(
        uint32_t Slot, BufferHandle Buffer,
        uint64_t Offset, uint64_t Range) = 0;

    virtual void BindStorageBuffer(
        uint32_t Slot, BufferHandle Buffer,
        uint64_t Offset, uint64_t Range) = 0;

    virtual void BindTexture(
        uint32_t Slot, TextureHandle Texture,
        SamplerHandle Sampler) = 0;

    virtual void BindStorageTexture(
        uint32_t Slot, TextureHandle Texture) = 0;

    // ---- Push Constants ----
    virtual void PushConstants(
        const void* Data, uint32_t Size, uint32_t Offset = 0) = 0;

    // ---- Draw Commands ----
    virtual void Draw(
        uint32_t VertexCount, uint32_t FirstVertex = 0,
        uint32_t InstanceCount = 1) = 0;

    virtual void DrawIndexed(
        uint32_t IndexCount, uint32_t FirstIndex = 0,
        int32_t VertexOffset = 0, uint32_t InstanceCount = 1) = 0;

    // Indirect draw: parameters read from a GPU buffer
    virtual void DrawIndirect(
        BufferHandle IndirectBuffer, uint64_t Offset,
        uint32_t DrawCount, uint32_t Stride) = 0;

    // ---- Compute ----
    virtual void DispatchCompute(
        uint32_t GroupX, uint32_t GroupY, uint32_t GroupZ = 1) = 0;

    // Indirect dispatch: parameters read from a GPU buffer
    virtual void DispatchIndirect(
        BufferHandle IndirectBuffer, uint64_t Offset) = 0;

    // ---- Copy Commands ----
    virtual void CopyBuffer(
        BufferHandle Src, BufferHandle Dst,
        uint64_t SrcOffset, uint64_t DstOffset, uint64_t Size) = 0;

    virtual void CopyBufferToTexture(
        BufferHandle Src, TextureHandle Dst,
        const Offset3D& DstOffset, const Extent3D& DstExtent) = 0;

    virtual void CopyTextureToBuffer(
        TextureHandle Src, BufferHandle Dst,
        const Offset3D& SrcOffset, const Extent3D& SrcExtent) = 0;

    // ---- GPU Queries / Timestamps ----
    virtual void ResetQueryPool(
        QueryPoolHandle Pool, uint32_t FirstQuery, uint32_t QueryCount) = 0;
    virtual void BeginQuery(
        QueryPoolHandle Pool, uint32_t QueryIndex) = 0;
    virtual void EndQuery(
        QueryPoolHandle Pool, uint32_t QueryIndex) = 0;
    virtual void WriteTimestamp(
        PipelineStage Stage, QueryPoolHandle Pool, uint32_t QueryIndex) = 0;

    // ---- Debug Markers ----
    virtual void BeginDebugLabel(
        const char* Label,
        std::array<float, 4> Color = {1.0f, 1.0f, 1.0f, 1.0f}) = 0;

    virtual void EndDebugLabel() = 0;
    virtual void InsertDebugMarker(const char* Marker) = 0;
};

} // namespace rhi
