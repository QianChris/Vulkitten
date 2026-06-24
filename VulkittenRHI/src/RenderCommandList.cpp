#include "rhi/RenderCommandList.hpp"
#include "rhi/IRenderDevice.hpp"
#include "rhi/ICommandBuffer.hpp"

namespace rhi {

// ============================================================
// Construction
// ============================================================

RenderCommandList::RenderCommandList(IRenderDevice& device, ICommandBuffer& cmd)
    : m_Device(device)
    , m_Cmd(cmd)
{
}

// ============================================================
// High-level convenience
// ============================================================

void RenderCommandList::DrawMesh(GeometryHandle geo, PipelineHandle pso,
                                  const std::vector<Binding>& bindings,
                                  const void* pushConstants, uint32_t pushSize)
{
    m_Cmd.BindPipeline(pso);
    m_Cmd.BindGeometry(geo);

    for (auto& b : bindings)
    {
        switch (b.BindType)
        {
            case Binding::Type::UniformBuffer:
                m_Cmd.BindUniformBuffer(b.Slot, b.Buffer, b.Offset, b.Range);
                break;
            case Binding::Type::StorageBuffer:
                m_Cmd.BindStorageBuffer(b.Slot, b.Buffer, b.Offset, b.Range);
                break;
            case Binding::Type::Texture:
                m_Cmd.BindTexture(b.Slot, b.Texture, b.Sampler);
                break;
            case Binding::Type::StorageTexture:
                m_Cmd.BindStorageTexture(b.Slot, b.Texture);
                break;
        }
    }

    if (pushConstants && pushSize > 0)
        m_Cmd.PushConstants(pushConstants, pushSize);

    m_Cmd.DrawIndexed(0, 0, 0, 1);  // Use geo's index count [HACK]
}

void RenderCommandList::DispatchCompute(PipelineHandle pso,
                                         const std::vector<Binding>& bindings,
                                         uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    m_Cmd.BindPipeline(pso);

    for (auto& b : bindings)
    {
        switch (b.BindType)
        {
            case Binding::Type::StorageBuffer:
                m_Cmd.BindStorageBuffer(b.Slot, b.Buffer, b.Offset, b.Range);
                break;
            case Binding::Type::StorageTexture:
                m_Cmd.BindStorageTexture(b.Slot, b.Texture);
                break;
            default:
                break;
        }
    }

    m_Cmd.DispatchCompute(groupX, groupY, groupZ);
}

// ============================================================
// Fluent API (chainable)
// ============================================================

RenderCommandList& RenderCommandList::BindPipeline(PipelineHandle pso)
{
    m_Cmd.BindPipeline(pso);
    return *this;
}

RenderCommandList& RenderCommandList::BindGeometry(GeometryHandle geo)
{
    m_Cmd.BindGeometry(geo);
    return *this;
}

RenderCommandList& RenderCommandList::BindUniformBuffer(uint32_t slot, BufferHandle buffer,
                                                          uint64_t offset, uint64_t range)
{
    m_Cmd.BindUniformBuffer(slot, buffer, offset, range);
    return *this;
}

RenderCommandList& RenderCommandList::BindStorageBuffer(uint32_t slot, BufferHandle buffer,
                                                          uint64_t offset, uint64_t range)
{
    m_Cmd.BindStorageBuffer(slot, buffer, offset, range);
    return *this;
}

RenderCommandList& RenderCommandList::BindTexture(uint32_t slot, TextureHandle texture,
                                                   SamplerHandle sampler)
{
    m_Cmd.BindTexture(slot, texture, sampler);
    return *this;
}

RenderCommandList& RenderCommandList::BindStorageTexture(uint32_t slot, TextureHandle texture)
{
    m_Cmd.BindStorageTexture(slot, texture);
    return *this;
}

RenderCommandList& RenderCommandList::PushConstants(const void* data, uint32_t size, uint32_t offset)
{
    m_Cmd.PushConstants(data, size, offset);
    return *this;
}

RenderCommandList& RenderCommandList::DrawIndexed(uint32_t indexCount, uint32_t firstIndex,
                                                   int32_t vertexOffset)
{
    m_Cmd.DrawIndexed(indexCount, firstIndex, vertexOffset);
    return *this;
}

RenderCommandList& RenderCommandList::Draw(uint32_t vertexCount, uint32_t firstVertex)
{
    m_Cmd.Draw(vertexCount, firstVertex);
    return *this;
}

RenderCommandList& RenderCommandList::Dispatch(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    m_Cmd.DispatchCompute(groupX, groupY, groupZ);
    return *this;
}

RenderCommandList& RenderCommandList::Barrier(TextureHandle texture,
                                               PipelineStage srcStage, AccessFlags srcAccess,
                                               PipelineStage dstStage, AccessFlags dstAccess,
                                               ImageLayout oldLayout, ImageLayout newLayout)
{
    m_Cmd.Barrier(texture, srcStage, srcAccess, dstStage, dstAccess, oldLayout, newLayout);
    return *this;
}

} // namespace rhi
