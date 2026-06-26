#pragma once

#include "rhi/Core/Handle.hpp"
#include "rhi/Core/Types.hpp"
#include "rhi/Core/Format.hpp"
#include "rhi/ResourceDescs.hpp"
#include "rhi/IBuffer.hpp"
#include "rhi/ITexture.hpp"
#include "rhi/IShader.hpp"
#include "rhi/IPipeline.hpp"
#include "rhi/IGeometry.hpp"
#include "rhi/ISampler.hpp"
#include "rhi/FrameContext.hpp"

#include <memory>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace rhi {

class IRenderDevice;
class ICommandBuffer;

// ============================================================
// ResourceManager — handle pool + resource lifecycle
//
// Owns the handle pool with ABA protection and all GPU
// resource metadata / RAII wrappers.
//
// Backend devices (GLDevice, VKDevice) call the AllocateSlot
// and Store* methods. Upper layers use the public create*/get*
// API.
// ============================================================

class ResourceManager
{
public:
    explicit ResourceManager();
    ~ResourceManager();

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    // ============================================================
    // Handle Pool (called by backend device create* methods)
    // ============================================================

    // Allocate a new slot. Returns (id, generation) pair.
    // The caller then creates the RAII resource and stores it
    // via Store* below.
    uint32_t AllocateSlot();

    // Free a slot (called on resource destruction).
    void FreeSlot(uint32_t id);

    // Check if a slot is alive (for handle validation)
    bool IsSlotAlive(uint32_t id) const;

    // Get generation for handle validation
    uint32_t GetGeneration(uint32_t id) const;

    // ============================================================
    // Resource Storage (called by backend device create* methods)
    // ============================================================

    void StoreBuffer(uint32_t id, std::unique_ptr<IBuffer> buffer);
    void StoreTexture(uint32_t id, std::unique_ptr<ITexture> texture);
    void StoreShader(uint32_t id, std::unique_ptr<IShader> shader);
    void StorePipeline(uint32_t id, std::unique_ptr<IPipeline> pipeline);
    void StoreGeometry(uint32_t id, std::unique_ptr<IGeometry> geometry);
    void StoreSampler(uint32_t id, std::unique_ptr<ISampler> sampler);

    // ============================================================
    // Resource Query (public, returns interface pointers)
    // ============================================================

    IBuffer*   GetBuffer(BufferHandle handle);
    ITexture*  GetTexture(TextureHandle handle);
    IShader*   GetShader(ShaderHandle handle);
    IPipeline* GetPipeline(PipelineHandle handle);
    IGeometry* GetGeometry(GeometryHandle handle);
    ISampler*  GetSampler(SamplerHandle handle);

    // ============================================================
    // Resource Destruction
    // ============================================================

    void DestroyBuffer(BufferHandle handle);
    void DestroyTexture(TextureHandle handle);
    void DestroyShader(ShaderHandle handle);
    void DestroyPipeline(PipelineHandle handle);
    void DestroyGeometry(GeometryHandle handle);
    void DestroySampler(SamplerHandle handle);

    // Destroy all resources (called on shutdown)
    void DestroyAll();

    // ============================================================
    // Metadata Maps (for RenderPass/Framebuffer and other
    // descriptor-only resources not backed by an interface)
    // ============================================================

    void StoreRenderPassDesc(uint32_t id, RenderPassDesc desc);
    const RenderPassDesc* GetRenderPassDesc(uint32_t id) const;

    void StoreFramebufferDesc(uint32_t id, FramebufferDesc desc);
    const FramebufferDesc* GetFramebufferDesc(uint32_t id) const;

private:
    // GpuSlot with ABA protection
    struct Slot
    {
        uint32_t Generation = 1;
        bool     Alive = false;
    };

    std::vector<Slot> m_Slots;
    std::vector<uint32_t> m_FreeIndices;

    // Typed resource pools (owned, RAII via virtual destructor)
    std::unordered_map<uint32_t, std::unique_ptr<IBuffer>>   m_Buffers;
    std::unordered_map<uint32_t, std::unique_ptr<ITexture>>  m_Textures;
    std::unordered_map<uint32_t, std::unique_ptr<IShader>>   m_Shaders;
    std::unordered_map<uint32_t, std::unique_ptr<IPipeline>> m_Pipelines;
    std::unordered_map<uint32_t, std::unique_ptr<IGeometry>> m_Geometries;
    std::unordered_map<uint32_t, std::unique_ptr<ISampler>>  m_Samplers;

    // Descriptor-only metadata
    std::unordered_map<uint32_t, RenderPassDesc>   m_RenderPassDescs;
    std::unordered_map<uint32_t, FramebufferDesc>  m_FramebufferDescs;
};

} // namespace rhi
