#include "rhi/ResourceManager.hpp"

namespace rhi {

// ============================================================
// Construction
// ============================================================

ResourceManager::ResourceManager()
{
    // Slot 0 = null handle
    m_Slots.push_back({1, false});
}

ResourceManager::~ResourceManager()
{
    DestroyAll();
}

// ============================================================
// Handle Pool
// ============================================================

uint32_t ResourceManager::AllocateSlot()
{
    if (!m_FreeIndices.empty())
    {
        uint32_t idx = m_FreeIndices.back();
        m_FreeIndices.pop_back();
        m_Slots[idx].Generation++;
        m_Slots[idx].Alive = true;
        return idx;
    }
    uint32_t idx = static_cast<uint32_t>(m_Slots.size());
    m_Slots.push_back({1, true});
    return idx;
}

void ResourceManager::FreeSlot(uint32_t id)
{
    if (id > 0 && id < m_Slots.size())
    {
        m_Slots[id].Alive = false;
        m_FreeIndices.push_back(id);
    }
}

bool ResourceManager::IsSlotAlive(uint32_t id) const
{
    return id > 0 && id < m_Slots.size() && m_Slots[id].Alive;
}

uint32_t ResourceManager::GetGeneration(uint32_t id) const
{
    if (id == 0 || id >= m_Slots.size()) return 1;
    return m_Slots[id].Generation;
}

// ============================================================
// Resource Storage
// ============================================================

void ResourceManager::StoreBuffer(uint32_t id, std::unique_ptr<IBuffer> buffer)
{
    m_Buffers[id] = std::move(buffer);
}

void ResourceManager::StoreTexture(uint32_t id, std::unique_ptr<ITexture> texture)
{
    m_Textures[id] = std::move(texture);
}

void ResourceManager::StoreShader(uint32_t id, std::unique_ptr<IShader> shader)
{
    m_Shaders[id] = std::move(shader);
}

void ResourceManager::StorePipeline(uint32_t id, std::unique_ptr<IPipeline> pipeline)
{
    m_Pipelines[id] = std::move(pipeline);
}

void ResourceManager::StoreGeometry(uint32_t id, std::unique_ptr<IGeometry> geometry)
{
    m_Geometries[id] = std::move(geometry);
}

void ResourceManager::StoreSampler(uint32_t id, std::unique_ptr<ISampler> sampler)
{
    m_Samplers[id] = std::move(sampler);
}

// ============================================================
// Resource Query
// ============================================================

IBuffer* ResourceManager::GetBuffer(BufferHandle handle)
{
    if (!handle.IsValid()) return nullptr;
    auto it = m_Buffers.find(handle.GetId());
    return (it != m_Buffers.end()) ? it->second.get() : nullptr;
}

ITexture* ResourceManager::GetTexture(TextureHandle handle)
{
    if (!handle.IsValid()) return nullptr;
    auto it = m_Textures.find(handle.GetId());
    return (it != m_Textures.end()) ? it->second.get() : nullptr;
}

IShader* ResourceManager::GetShader(ShaderHandle handle)
{
    if (!handle.IsValid()) return nullptr;
    auto it = m_Shaders.find(handle.GetId());
    return (it != m_Shaders.end()) ? it->second.get() : nullptr;
}

IPipeline* ResourceManager::GetPipeline(PipelineHandle handle)
{
    if (!handle.IsValid()) return nullptr;
    auto it = m_Pipelines.find(handle.GetId());
    return (it != m_Pipelines.end()) ? it->second.get() : nullptr;
}

IGeometry* ResourceManager::GetGeometry(GeometryHandle handle)
{
    if (!handle.IsValid()) return nullptr;
    auto it = m_Geometries.find(handle.GetId());
    return (it != m_Geometries.end()) ? it->second.get() : nullptr;
}

ISampler* ResourceManager::GetSampler(SamplerHandle handle)
{
    if (!handle.IsValid()) return nullptr;
    auto it = m_Samplers.find(handle.GetId());
    return (it != m_Samplers.end()) ? it->second.get() : nullptr;
}

// ============================================================
// Resource Destruction
// ============================================================

void ResourceManager::DestroyBuffer(BufferHandle handle)
{
    if (!handle.IsValid()) return;
    m_Buffers.erase(handle.GetId());
    FreeSlot(handle.GetId());
}

void ResourceManager::DestroyTexture(TextureHandle handle)
{
    if (!handle.IsValid()) return;
    m_Textures.erase(handle.GetId());
    FreeSlot(handle.GetId());
}

void ResourceManager::DestroyShader(ShaderHandle handle)
{
    if (!handle.IsValid()) return;
    m_Shaders.erase(handle.GetId());
    FreeSlot(handle.GetId());
}

void ResourceManager::DestroyPipeline(PipelineHandle handle)
{
    if (!handle.IsValid()) return;
    m_Pipelines.erase(handle.GetId());
    FreeSlot(handle.GetId());
}

void ResourceManager::DestroyGeometry(GeometryHandle handle)
{
    if (!handle.IsValid()) return;
    m_Geometries.erase(handle.GetId());
    FreeSlot(handle.GetId());
}

void ResourceManager::DestroySampler(SamplerHandle handle)
{
    if (!handle.IsValid()) return;
    m_Samplers.erase(handle.GetId());
    FreeSlot(handle.GetId());
}

void ResourceManager::DestroyAll()
{
    // unique_ptr destructors handle the RAII cleanup automatically
    m_Buffers.clear();
    m_Textures.clear();
    m_Shaders.clear();
    m_Pipelines.clear();
    m_Geometries.clear();
    m_Samplers.clear();
    m_RenderPassDescs.clear();
    m_FramebufferDescs.clear();

    // Reset slot pool (keep slot 0 = null)
    m_Slots.clear();
    m_Slots.push_back({1, false});
    m_FreeIndices.clear();
}

// ============================================================
// Metadata Maps
// ============================================================

void ResourceManager::StoreRenderPassDesc(uint32_t id, RenderPassDesc desc)
{
    m_RenderPassDescs[id] = std::move(desc);
}

const RenderPassDesc* ResourceManager::GetRenderPassDesc(uint32_t id) const
{
    auto it = m_RenderPassDescs.find(id);
    return (it != m_RenderPassDescs.end()) ? &it->second : nullptr;
}

void ResourceManager::StoreFramebufferDesc(uint32_t id, FramebufferDesc desc)
{
    m_FramebufferDescs[id] = std::move(desc);
}

const FramebufferDesc* ResourceManager::GetFramebufferDesc(uint32_t id) const
{
    auto it = m_FramebufferDescs.find(id);
    return (it != m_FramebufferDescs.end()) ? &it->second : nullptr;
}

} // namespace rhi
