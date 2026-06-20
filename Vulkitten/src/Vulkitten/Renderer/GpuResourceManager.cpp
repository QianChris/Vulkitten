#include "vktpch.h"
#include "GpuResourceManager.h"

namespace Vulkitten {

// ============================================================
// Slot Allocation (free-list with generation bump)
// ============================================================

uint32_t GpuResourceManager::AllocateSlot()
{
    // Reuse a freed slot if available
    if (!m_FreeIndices.empty())
    {
        uint32_t index = m_FreeIndices.back();
        m_FreeIndices.pop_back();

        auto& slot = m_Slots[index];
        slot.alive = true;
        slot.generation++;          // Invalidate old handles pointing here
        slot.type = GpuResourceSlot::Type::None;
        slot.deferred = true;
        slot.gpuHandle = 0;
        return index;
    }

    // Allocate a new slot
    GpuResourceSlot slot;
    slot.alive = true;
    slot.generation = 0;
    m_Slots.push_back(slot);
    return uint32_t(m_Slots.size() - 1);
}

// ============================================================
// Handle Validation
// ============================================================

bool GpuResourceManager::ValidateHandle(uint32_t index, uint16_t generation) const
{
    if (index >= m_Slots.size())
        return false;

    const auto& slot = m_Slots[index];
    return slot.alive && slot.generation == generation;
}

// ============================================================
// Resource Creation
// ============================================================

uint64_t GpuResourceManager::CreateTexture(const GpuTextureDesc& desc, const std::string& debugName)
{
    uint32_t index = AllocateSlot();

    auto& slot = m_Slots[index];
    slot.type = GpuResourceSlot::Type::Texture;
    slot.textureDesc = desc;
    slot.deferred = true;
    slot.gpuHandle = 0;
    slot.debugName = debugName;

    return MakeHandle(index, slot.generation);
}

uint64_t GpuResourceManager::CreateBuffer(const GpuBufferDesc& desc, const std::string& debugName)
{
    uint32_t index = AllocateSlot();

    auto& slot = m_Slots[index];
    slot.type = GpuResourceSlot::Type::Buffer;
    slot.bufferDesc = desc;
    slot.deferred = true;
    slot.gpuHandle = 0;
    slot.debugName = debugName;

    return MakeHandle(index, slot.generation);
}

// ============================================================
// Resource Lookup (triggers deferred allocation)
// ============================================================

GpuResourceSlot* GpuResourceManager::GetTexture(uint64_t handle)
{
    uint32_t  index = GetIndex(handle);
    uint16_t  gen   = GetGeneration(handle);

    if (!ValidateHandle(index, gen))
        return nullptr;

    auto& slot = m_Slots[index];
    if (slot.type != GpuResourceSlot::Type::Texture)
        return nullptr;

    if (slot.deferred)
    {
        // Deferred GPU allocation — placeholder for now.
        // In the future: glGenTextures / vkCreateImage
        slot.deferred = false;
        VKT_CORE_INFO("GpuResourceManager: deferred texture creation '{0}' ({1}x{2})",
                      slot.debugName, slot.textureDesc.Width, slot.textureDesc.Height);
    }

    slot.lastUsedFrame = m_CurrentFrame;
    return &slot;
}

GpuResourceSlot* GpuResourceManager::GetBuffer(uint64_t handle)
{
    uint32_t  index = GetIndex(handle);
    uint16_t  gen   = GetGeneration(handle);

    if (!ValidateHandle(index, gen))
        return nullptr;

    auto& slot = m_Slots[index];
    if (slot.type != GpuResourceSlot::Type::Buffer)
        return nullptr;

    if (slot.deferred)
    {
        // Deferred GPU allocation — placeholder for now.
        slot.deferred = false;
        VKT_CORE_INFO("GpuResourceManager: deferred buffer creation '{0}' ({1} bytes)",
                      slot.debugName, slot.bufferDesc.Size);
    }

    slot.lastUsedFrame = m_CurrentFrame;
    return &slot;
}

// ============================================================
// Low-level Slot Access
// ============================================================

GpuResourceSlot* GpuResourceManager::GetSlot(uint32_t index)
{
    if (index >= m_Slots.size())
        return nullptr;
    return &m_Slots[index];
}

// ============================================================
// Resource Destruction
// ============================================================

void GpuResourceManager::DestroyResource(uint64_t handle)
{
    uint32_t  index = GetIndex(handle);
    uint16_t  gen   = GetGeneration(handle);

    if (!ValidateHandle(index, gen))
        return;

    auto& slot = m_Slots[index];
    slot.alive = false;
    slot.type  = GpuResourceSlot::Type::None;
    // Future: glDeleteTextures / vkDestroyImage using slot.gpuHandle

    m_FreeIndices.push_back(index);
}

// ============================================================
// Frame Management
// ============================================================

void GpuResourceManager::TickFrame()
{
    m_CurrentFrame++;
}

void GpuResourceManager::Gc(uint32_t maxFramesInFlight)
{
    // Collect indices of resources eligible for garbage collection.
    // A resource is eligible when:
    //   1. It hasn't been accessed for `maxFramesInFlight` frames.
    //   2. It is alive (not already destroyed).
    // For now, we rely on the frame gap heuristic.
    // When Task 5 integrates Ref<Texture2D>/Ref<Buffer>, external
    // reference counting will be checked here before destruction.
    std::vector<uint64_t> toDestroy;

    for (uint32_t i = 0; i < m_Slots.size(); i++)
    {
        auto& slot = m_Slots[i];
        if (!slot.alive)
            continue;

        uint32_t unusedFrames = m_CurrentFrame - slot.lastUsedFrame;
        if (unusedFrames > maxFramesInFlight)
        {
            toDestroy.push_back(MakeHandle(i, slot.generation));
        }
    }

    for (uint64_t handle : toDestroy)
    {
        uint32_t index = GetIndex(handle);
        VKT_CORE_INFO("GpuResourceManager::Gc — destroying unused resource '{0}' "
                      "(unused for {1} frames)",
                      m_Slots[index].debugName,
                      m_CurrentFrame - m_Slots[index].lastUsedFrame);
        DestroyResource(handle);
    }

    if (!toDestroy.empty())
        VKT_CORE_INFO("GpuResourceManager::Gc — cleaned up {0} resources", toDestroy.size());
}

} // namespace Vulkitten
