#pragma once

#include "rhi/ICommandBuffer.hpp"
#include "rhi/ResourceDescs.hpp"
#include "rhi/Core/Handle.hpp"
#include "rhi/Core/Types.hpp"

#include <cstdint>
#include <array>
#include <vector>
#include <string>
#include <cstring>

namespace rhi {
namespace test {

// ============================================================
// CallRecord - stores a single command call with parameters
// ============================================================

enum class CallType : uint32_t
{
    Begin,
    End,
    Barrier_Buffer,
    Barrier_Texture,
    BeginRenderPass,
    EndRenderPass,
    BindPipeline,
    BindGeometry,
    BindUniformBuffer,
    BindStorageBuffer,
    BindTexture,
    BindStorageTexture,
    PushConstants,
    Draw,
    DrawIndexed,
    DrawIndirect,
    DispatchCompute,
    DispatchIndirect,
    CopyBuffer,
    CopyBufferToTexture,
    CopyTextureToBuffer,
    ResetQueryPool,
    BeginQuery,
    EndQuery,
    WriteTimestamp,
    BeginDebugLabel,
    EndDebugLabel,
    InsertDebugMarker,
};

struct CallRecord
{
    CallType Type;

    // Draw parameters
    uint32_t VertexCount = 0;
    uint32_t FirstVertex = 0;
    uint32_t InstanceCount = 1;

    // DrawIndexed parameters
    uint32_t IndexCount = 0;
    uint32_t FirstIndex = 0;
    int32_t  VertexOffset = 0;

    // DrawIndirect parameters
    uint32_t IndirectDrawCount = 0;
    uint32_t Stride = 0;
    uint64_t BufferOffset = 0;

    // Dispatch parameters
    uint32_t GroupX = 0;
    uint32_t GroupY = 0;
    uint32_t GroupZ = 1;

    // Handle storage
    uint32_t BufferHandleId = 0;
    uint32_t PipelineHandleId = 0;
    uint32_t GeometryHandleId = 0;
    uint32_t TextureHandleId = 0;
    uint32_t SamplerHandleId = 0;
    uint32_t Slot = 0;

    // Push constants
    std::vector<uint8_t> PushData;
    uint32_t PushSize = 0;
    uint32_t PushOffset = 0;

    // Debug label
    std::string DebugLabel;
    std::array<float, 4> DebugColor = {1, 1, 1, 1};
};

// ============================================================
// MockCommandBuffer - records all calls for verification
//
// Usage:
//   MockCommandBuffer cmd;
//   cmd.Draw(3, 0, 1);
//   ASSERT_EQ(cmd.GetCalls().size(), 1u);
//   ASSERT_EQ(cmd.GetCalls()[0].Type, CallType::Draw);
//   ASSERT_EQ(cmd.GetCalls()[0].VertexCount, 3u);
// ============================================================

class MockCommandBuffer : public ICommandBuffer
{
public:
    MockCommandBuffer() = default;
    ~MockCommandBuffer() override = default;

    // ---- Call history access ----
    const std::vector<CallRecord>& GetCalls() const { return m_Calls; }
    void ClearCalls() { m_Calls.clear(); }

    // Helper: get last call of a specific type
    const CallRecord* FindLastCall(CallType type) const
    {
        for (auto it = m_Calls.rbegin(); it != m_Calls.rend(); ++it)
            if (it->Type == type) return &(*it);
        return nullptr;
    }

    // Helper: count calls of a specific type
    size_t CountCalls(CallType type) const
    {
        size_t count = 0;
        for (auto& c : m_Calls)
            if (c.Type == type) ++count;
        return count;
    }

    // ---- ICommandBuffer Interface ----

    void Begin() override { m_Calls.push_back({CallType::Begin}); }
    void End() override { m_Calls.push_back({CallType::End}); }

    CommandBufferLevel GetLevel() const override { return CommandBufferLevel::Primary; }

    void Barrier(PipelineStage, AccessFlags, PipelineStage, AccessFlags) override
    {
        m_Calls.push_back({CallType::Barrier_Buffer});
    }

    void Barrier(TextureHandle, PipelineStage, AccessFlags,
                 PipelineStage, AccessFlags, ImageLayout, ImageLayout) override
    {
        m_Calls.push_back({CallType::Barrier_Texture});
    }

    void BeginRenderPass(RenderPassHandle, FramebufferHandle,
                         const Rect2D&, const ClearValue*, uint32_t) override
    {
        m_Calls.push_back({CallType::BeginRenderPass});
    }

    void EndRenderPass() override { m_Calls.push_back({CallType::EndRenderPass}); }

    void BindPipeline(PipelineHandle pipeline) override
    {
        CallRecord r{CallType::BindPipeline};
        r.PipelineHandleId = pipeline.GetId();
        m_Calls.push_back(r);
    }

    void BindGeometry(GeometryHandle geometry) override
    {
        CallRecord r{CallType::BindGeometry};
        r.GeometryHandleId = geometry.GetId();
        m_Calls.push_back(r);
    }

    void BindUniformBuffer(uint32_t slot, BufferHandle buffer,
                           uint64_t offset, uint64_t range) override
    {
        CallRecord r{CallType::BindUniformBuffer};
        r.Slot = slot;
        r.BufferHandleId = buffer.GetId();
        r.BufferOffset = offset;
        r.PushSize = static_cast<uint32_t>(range);
        m_Calls.push_back(r);
    }

    void BindStorageBuffer(uint32_t slot, BufferHandle buffer,
                           uint64_t offset, uint64_t range) override
    {
        CallRecord r{CallType::BindStorageBuffer};
        r.Slot = slot;
        r.BufferHandleId = buffer.GetId();
        r.BufferOffset = offset;
        r.PushSize = static_cast<uint32_t>(range);
        m_Calls.push_back(r);
    }

    void BindTexture(uint32_t slot, TextureHandle texture, SamplerHandle sampler) override
    {
        CallRecord r{CallType::BindTexture};
        r.Slot = slot;
        r.TextureHandleId = texture.GetId();
        r.SamplerHandleId = sampler.GetId();
        m_Calls.push_back(r);
    }

    void BindStorageTexture(uint32_t slot, TextureHandle texture) override
    {
        CallRecord r{CallType::BindStorageTexture};
        r.Slot = slot;
        r.TextureHandleId = texture.GetId();
        m_Calls.push_back(r);
    }

    void PushConstants(const void* data, uint32_t size, uint32_t offset = 0) override
    {
        CallRecord r{CallType::PushConstants};
        r.PushSize = size;
        r.PushOffset = offset;
        if (data && size > 0)
        {
            r.PushData.resize(size);
            std::memcpy(r.PushData.data(), data, size);
        }
        m_Calls.push_back(r);
    }

    // ---- Draw Commands ----
    void Draw(uint32_t vertexCount, uint32_t firstVertex = 0,
              uint32_t instanceCount = 1) override
    {
        CallRecord r{CallType::Draw};
        r.VertexCount = vertexCount;
        r.FirstVertex = firstVertex;
        r.InstanceCount = instanceCount;
        m_Calls.push_back(r);
    }

    void DrawIndexed(uint32_t indexCount, uint32_t firstIndex = 0,
                     int32_t vertexOffset = 0, uint32_t instanceCount = 1) override
    {
        CallRecord r{CallType::DrawIndexed};
        r.IndexCount = indexCount;
        r.FirstIndex = firstIndex;
        r.VertexOffset = vertexOffset;
        r.InstanceCount = instanceCount;
        m_Calls.push_back(r);
    }

    void DrawIndirect(BufferHandle indirectBuffer, uint64_t offset,
                      uint32_t drawCount, uint32_t stride) override
    {
        CallRecord r{CallType::DrawIndirect};
        r.BufferHandleId = indirectBuffer.GetId();
        r.BufferOffset = offset;
        r.IndirectDrawCount = drawCount;
        r.Stride = stride;
        m_Calls.push_back(r);
    }

    // ---- Compute ----
    void DispatchCompute(uint32_t groupX, uint32_t groupY, uint32_t groupZ = 1) override
    {
        CallRecord r{CallType::DispatchCompute};
        r.GroupX = groupX;
        r.GroupY = groupY;
        r.GroupZ = groupZ;
        m_Calls.push_back(r);
    }

    void DispatchIndirect(BufferHandle indirectBuffer, uint64_t offset) override
    {
        CallRecord r{CallType::DispatchIndirect};
        r.BufferHandleId = indirectBuffer.GetId();
        r.BufferOffset = offset;
        m_Calls.push_back(r);
    }

    // ---- Copy Commands ----
    void CopyBuffer(BufferHandle, BufferHandle, uint64_t, uint64_t, uint64_t) override
    {
        m_Calls.push_back({CallType::CopyBuffer});
    }
    void CopyBufferToTexture(BufferHandle, TextureHandle, const Offset3D&, const Extent3D&) override
    {
        m_Calls.push_back({CallType::CopyBufferToTexture});
    }
    void CopyTextureToBuffer(TextureHandle, BufferHandle, const Offset3D&, const Extent3D&) override
    {
        m_Calls.push_back({CallType::CopyTextureToBuffer});
    }

    // ---- Queries ----
    void ResetQueryPool(QueryPoolHandle, uint32_t, uint32_t) override
    {
        m_Calls.push_back({CallType::ResetQueryPool});
    }
    void BeginQuery(QueryPoolHandle, uint32_t) override
    {
        m_Calls.push_back({CallType::BeginQuery});
    }
    void EndQuery(QueryPoolHandle, uint32_t) override
    {
        m_Calls.push_back({CallType::EndQuery});
    }
    void WriteTimestamp(PipelineStage, QueryPoolHandle, uint32_t) override
    {
        m_Calls.push_back({CallType::WriteTimestamp});
    }

    // ---- Debug Markers ----
    void BeginDebugLabel(const char* label,
                         std::array<float, 4> color = {1.0f, 1.0f, 1.0f, 1.0f}) override
    {
        CallRecord r{CallType::BeginDebugLabel};
        r.DebugLabel = label ? label : "";
        r.DebugColor = color;
        m_Calls.push_back(r);
    }

    void EndDebugLabel() override { m_Calls.push_back({CallType::EndDebugLabel}); }

    void InsertDebugMarker(const char* marker) override
    {
        CallRecord r{CallType::InsertDebugMarker};
        r.DebugLabel = marker ? marker : "";
        m_Calls.push_back(r);
    }

private:
    std::vector<CallRecord> m_Calls;
};

} // namespace test
} // namespace rhi
