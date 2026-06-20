#pragma once

#include "Vulkitten/Core/Core.h"

#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>
#include <memory>

namespace Vulkitten {

// ============================================================
// Resource Descriptors (minimal — expanded in later tasks)
// ============================================================

struct GpuTextureDesc
{
    uint32_t Width = 0;
    uint32_t Height = 0;
    // Future: format, mipLevels, samples, usage, etc.
};

struct GpuBufferDesc
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
    uint32_t lastUsedFrame = 0; // Frame number of last GetTexture/GetBuffer access

    GpuTextureDesc textureDesc;
    GpuBufferDesc  bufferDesc;
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

    uint64_t CreateTexture(const GpuTextureDesc& desc, const std::string& debugName = "");
    uint64_t CreateBuffer(const GpuBufferDesc& desc, const std::string& debugName = "");

    // ---- Resource Lookup (triggers deferred allocation on first call) ----

    GpuResourceSlot* GetTexture(uint64_t handle);
    GpuResourceSlot* GetBuffer(uint64_t handle);

    // ---- Low-level Slot Access ----

    GpuResourceSlot* GetSlot(uint32_t index);

    // ---- External Reference Tracking ----

    // Register a weak_ptr that tracks an external Ref (Texture2D/Buffer).
    // The GC will skip this resource as long as the external Ref is alive.
    void TrackExternalRef(uint64_t handle, const std::weak_ptr<void>& tracker);

    // Set the platform GPU handle on an already-created slot (for
    // resources created externally via Texture2D::Create / Buffer::Create).
    void SetGpuHandle(uint64_t handle, uint64_t gpuHandle);

    // ---- Resource Destruction ----

    void DestroyResource(uint64_t handle);

    // ---- Handle Encoding (index + generation → uint64_t) ----

    static uint32_t GetIndex(uint64_t handle)      { return uint32_t(handle & 0xFFFFFFFF); }
    static uint16_t GetGeneration(uint64_t handle) { return uint16_t(handle >> 32); }
    static uint64_t MakeHandle(uint32_t index, uint16_t generation)
    {
        return (uint64_t(generation) << 32) | uint64_t(index);
    }

    // ---- Frame Management ----

    // Called each frame to advance the internal frame counter.
    void TickFrame();

    // Garbage-collect resources that haven't been accessed for
    // maxFramesInFlight frames and have no external references.
    void Gc(uint32_t maxFramesInFlight);

    // ---- Queries ----

    size_t GetResourceCount() const { return m_Slots.size() - m_FreeIndices.size(); }
    size_t GetSlotCount()    const { return m_Slots.size(); }

private:
    uint32_t AllocateSlot();
    bool     ValidateHandle(uint32_t index, uint16_t generation) const;

    std::vector<GpuResourceSlot> m_Slots;
    std::vector<uint32_t>        m_FreeIndices;   // Recycled slot indices
    uint32_t                     m_CurrentFrame = 0;

    // External Ref tracking: slot index → weak_ptr aliasing the Texture2D/Buffer shared_ptr.
    std::unordered_map<uint32_t, std::weak_ptr<void>> m_ExternalTrackers;
};

} // namespace Vulkitten
