#pragma once

#include "rhi/Core/Export.hpp"
#include "rhi/Core/Handle.hpp"
#include "rhi/Core/Types.hpp"
#include "rhi/ResourceDescs.hpp"
#include "rhi/ICommandBuffer.hpp"

#include <vector>
#include <cstddef>

namespace rhi {

class IRenderDevice;
class ResourceManager;

// ============================================================
// Binding - describes a single resource binding for drawMesh
// ============================================================

struct Binding
{
    enum class Type { UniformBuffer, StorageBuffer, Texture, StorageTexture };

    Type     BindType = Type::Texture;
    uint32_t Slot = 0;

    union {
        BufferHandle  Buffer;
        TextureHandle Texture;
    };
    uint64_t      Offset = 0;
    uint64_t      Range = 0;      // buffer only
    SamplerHandle Sampler;        // texture only (Type::Texture)
};

// ============================================================
// RenderCommandList - thin per-frame draw adapter
//
// Responsibilities:
//   1. Holds this frame's ICommandBuffer reference
//   2. Provides high-level convenience (drawMesh)
//   3. Exposes device/cmd/resources for direct low-level access
//   4. Chainable fluent API
//
// Lifecycle: created by Renderer::BeginFrame(),
//            used by passes, destroyed by Renderer::EndFrame().
// ============================================================

class RHI_API RenderCommandList
{
public:
    RenderCommandList(IRenderDevice& device, ResourceManager& rm, ICommandBuffer& cmd);

    // ---- Direct access (escape hatch) ----
    ICommandBuffer&  Cmd()       { return m_Cmd; }
    IRenderDevice&   Device()    { return m_Device; }
    ResourceManager& Resources() { return m_Rm; }

    // ---- High-level convenience ----
    void DrawMesh(GeometryHandle geo, PipelineHandle pso,
                  const std::vector<Binding>& bindings,
                  const void* pushConstants = nullptr, uint32_t pushSize = 0);

    void DispatchCompute(PipelineHandle pso,
                         const std::vector<Binding>& bindings,
                         uint32_t groupX, uint32_t groupY, uint32_t groupZ = 1);

    // ---- Fluent API (chainable, returns *this) ----
    RenderCommandList& BindPipeline(PipelineHandle pso);
    RenderCommandList& BindGeometry(GeometryHandle geo);
    RenderCommandList& BindUniformBuffer(uint32_t slot, BufferHandle buffer,
                                         uint64_t offset, uint64_t range);
    RenderCommandList& BindStorageBuffer(uint32_t slot, BufferHandle buffer,
                                         uint64_t offset, uint64_t range);
    RenderCommandList& BindTexture(uint32_t slot, TextureHandle texture,
                                   SamplerHandle sampler);
    RenderCommandList& BindStorageTexture(uint32_t slot, TextureHandle texture);
    RenderCommandList& PushConstants(const void* data, uint32_t size, uint32_t offset = 0);
    RenderCommandList& DrawIndexed(uint32_t indexCount, uint32_t firstIndex = 0,
                                   int32_t vertexOffset = 0);
    RenderCommandList& Draw(uint32_t vertexCount, uint32_t firstVertex = 0);
    RenderCommandList& DrawIndirect(BufferHandle indirectBuffer, uint64_t offset,
                                    uint32_t drawCount, uint32_t stride);
    RenderCommandList& Dispatch(uint32_t groupX, uint32_t groupY, uint32_t groupZ = 1);
    RenderCommandList& DispatchIndirect(BufferHandle indirectBuffer, uint64_t offset);
    RenderCommandList& Barrier(PipelineStage srcStage, AccessFlags srcAccess,
                               PipelineStage dstStage, AccessFlags dstAccess);
    RenderCommandList& Barrier(TextureHandle texture,
                               PipelineStage srcStage, AccessFlags srcAccess,
                               PipelineStage dstStage, AccessFlags dstAccess,
                               ImageLayout oldLayout, ImageLayout newLayout);

private:
    IRenderDevice&  m_Device;
    ResourceManager& m_Rm;
    ICommandBuffer& m_Cmd;
};

} // namespace rhi
