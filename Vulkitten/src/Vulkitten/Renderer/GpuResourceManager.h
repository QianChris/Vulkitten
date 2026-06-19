#pragma once

#include "Vulkitten/Core/Core.h"

#include <vector>
#include <string>
#include <cstdint>

namespace Vulkitten {

// ============================================================
// Resource Descriptors (minimal — expanded in later tasks)
// ============================================================

struct TextureDesc
{
    uint32_t Width = 0;
    uint32_t Height = 0;
    // Future: format, mipLevels, samples, usage, etc.
};

struct BufferDesc
{
    size_t Size = 0;
    // Future: usage flags, memory properties, etc.
};

// ============================================================
// Internal Resource Slot
// ============================================================

struct GpuResourceSlot
{
    enum class Type { None, Texture, Buffer } type = Type::None;

    uint16_t generation = 0;
    bool alive = false;
    bool deferred = true;       // Waiting for first Get to allocate GPU resource
    uint64_t gpuHandle = 0;    // Platform resource (GLuint / VkImage / VkBuffer)

    TextureDesc textureDesc;
    BufferDesc  bufferDesc;
    std::string debugName;
};

// ============================================================
// GpuResourceManager — Centralized VRAM resource manager.
//
// All GPU resources are allocated through this manager and
// referenced via uint64_t handles (encoding index + generation).
// Supports deferred creation: the descriptor is recorded at
// Create time, but actual GPU allocation is deferred until
// the first GetTexture/GetBuffer call.
//
// Existing Texture/Buffer classes (Ref<Texture2D>, etc.) are
// NOT migrated yet — this is the skeleton that will eventually
// replace them.
// ============================================================

class VKT_API GpuResourceManager
{
public:
    GpuResourceManager() = default;
    ~GpuResourceManager() = default;

    // ---- Resource Creation (returns handle, defers GPU allocation) ----

    uint64_t CreateTexture(const TextureDesc& desc, const std::string& debugName = "");
    uint64_t CreateBuffer(const BufferDesc& desc, const std::string& debugName = "");

    // ---- Resource Lookup (triggers deferred allocation on first call) ----

    GpuResourceSlot* GetTexture(uint64_t handle);
    GpuResourceSlot* GetBuffer(uint64_t handle);

    // ---- Low-level Slot Access ----

    GpuResourceSlot* GetSlot(uint32_t index);

    // ---- Resource Destruction ----

    void DestroyResource(uint64_t handle);

    // ---- Handle Encoding (index + generation → uint64_t) ----

    static uint32_t GetIndex(uint64_t handle)      { return uint32_t(handle & 0xFFFFFFFF); }
    static uint16_t GetGeneration(uint64_t handle) { return uint16_t(handle >> 32); }
    static uint64_t MakeHandle(uint32_t index, uint16_t generation)
    {
        return (uint64_t(generation) << 32) | uint64_t(index);
    }

    // ---- Queries ----

    size_t GetResourceCount() const { return m_Slots.size() - m_FreeIndices.size(); }
    size_t GetSlotCount()    const { return m_Slots.size(); }

private:
    uint32_t AllocateSlot();
    bool     ValidateHandle(uint32_t index, uint16_t generation) const;

    std::vector<GpuResourceSlot> m_Slots;
    std::vector<uint32_t>        m_FreeIndices;   // Recycled slot indices
};

} // namespace Vulkitten
