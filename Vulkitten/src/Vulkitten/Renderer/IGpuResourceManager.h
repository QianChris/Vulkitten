#pragma once

#include "Vulkitten/Core/Core.h"

#include <cstdint>
#include <string>
#include <memory>
#include <filesystem>

namespace Vulkitten {

class Shader;

// ============================================================
// Resource Descriptors (platform-agnostic)
// ============================================================

struct GpuTextureDesc
{
    uint32_t Width = 0;
    uint32_t Height = 0;
};

struct GpuBufferDesc
{
    size_t Size = 0;
};

// ============================================================
// Internal Resource Slot
// ============================================================

struct GpuResourceSlot
{
    enum class Type { None, Texture, Buffer } type = Type::None;
    uint16_t generation = 0;
    bool alive = false;
    bool deferred = true;
    uint64_t gpuHandle = 0;
    uint32_t lastUsedFrame = 0;
    GpuTextureDesc textureDesc;
    GpuBufferDesc  bufferDesc;
    std::string debugName;
};

// ============================================================
// ShaderData - preprocessed shader source and metadata
// ============================================================
struct ShaderData
{
    std::string VirtualPath;
    std::string ResolvedPath;
    std::string PreprocessedSource;
    bool        IsLoaded = false;
};

// ============================================================
// IGpuResourceManager - centralized GPU resource management.
//
// All GPU resources must be created through this interface.
// Resources are referenced via uint64_t handles (encoding
// 32-bit index + 16-bit generation for safe access).
//
// The manager owns resource lifecycle: creation, lookup,
// deferred allocation, and garbage collection.
//
// Concrete implementations: GpuResourceManager (OpenGL),
// VkGpuResourceManager (Vulkan).
// ============================================================
class IGpuResourceManager
{
public:
    virtual ~IGpuResourceManager() = default;

    // ---- Resource Creation (returns safe handles) ----

    // Create a texture resource. The descriptor is recorded;
    // actual GPU allocation may be deferred until first lookup.
    virtual uint64_t CreateTexture(const GpuTextureDesc& desc, const std::string& debugName = "") = 0;

    // Create a buffer resource (vertex, index, uniform, storage).
    virtual uint64_t CreateBuffer(const GpuBufferDesc& desc, const std::string& debugName = "") = 0;

    // Create a shader resource from source/preprocessed data.
    // Returns a handle to the compiled shader module.
    virtual uint64_t CreateShader(const std::string& name, const std::string& source) = 0;

    // Create a shader from .spv files: loads name.vert.spv + name.frag.spv
    virtual uint64_t CreateShaderFromSpv(const std::string& name, const std::string& virtualPath) = 0;

    // Get a usable Shader reference by handle (shared ownership)
    virtual Ref<Shader> GetShader(uint64_t handle) = 0;

    // Create a graphics pipeline (VkPipeline / GL program).
    // The pipeline encapsulates shader stages, vertex layout,
    // rasterizer state, and blend state.
    virtual uint64_t CreatePipeline(const void* pipelineDesc) = 0;

    // Create a geometry resource (vertex + index buffers).
    // Returns a handle to the complete drawable geometry.
    virtual uint64_t CreateGeometry(const void* geometryDesc) = 0;

    // ---- Shader Loading (formerly ShaderManager) ----

    // Load a shader from a virtual path, preprocess #include directives,
    // and return a handle for later retrieval.
    virtual uint64_t LoadShader(const std::string& virtualPath) = 0;

    // Retrieve preprocessed shader data by handle.
    virtual const ShaderData* GetShaderData(uint64_t handle) const = 0;

    // ---- Resource Lookup ----

    virtual GpuResourceSlot* GetTexture(uint64_t handle) = 0;
    virtual GpuResourceSlot* GetBuffer(uint64_t handle) = 0;
    virtual GpuResourceSlot* GetSlot(uint32_t index) = 0;

    // ---- External Reference Tracking ----

    // Register a weak_ptr that tracks an external Ref (Texture2D/Buffer).
    // The GC will skip this resource as long as the external Ref is alive.
    virtual void TrackExternalRef(uint64_t handle, const std::weak_ptr<void>& tracker) = 0;

    // Set the platform GPU handle on an already-created slot (for
    // resources created externally via Texture2D::Create / Buffer::Create).
    virtual void SetGpuHandle(uint64_t handle, uint64_t gpuHandle) = 0;

    // ---- Resource Destruction ----

    virtual void DestroyResource(uint64_t handle) = 0;

    // ---- Frame Management ----

    // Advance the internal frame counter (called each frame).
    virtual void TickFrame() = 0;

    // Garbage-collect resources unused for maxFramesInFlight frames.
    virtual void Gc(uint32_t maxFramesInFlight) = 0;

    // ---- Queries ----

    virtual size_t GetResourceCount() const = 0;
};

} // namespace Vulkitten
