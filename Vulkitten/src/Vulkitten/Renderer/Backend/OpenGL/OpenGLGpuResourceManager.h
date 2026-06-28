#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Renderer/IGpuResourceManager.h"
#include "Vulkitten/Renderer/Shader.h"

#include "Vulkitten/RHI/Handle.hpp"

#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>
#include <memory>

namespace Vulkitten {

class FileSystem;

// ============================================================
// OpenGLGpuResourceManager - OpenGL implementation of IGpuResourceManager.
//
// All GPU resources are allocated through this manager and
// referenced via uint64_t handles (encoding index + generation).
// Supports deferred creation, external ref tracking, and GC.
// ============================================================
class OpenGLGpuResourceManager : public IGpuResourceManager
{
public:
    explicit OpenGLGpuResourceManager(FileSystem& fileSystem);
    ~OpenGLGpuResourceManager() = default;

    // ---- Resource Creation ----
    uint64_t CreateTexture(const GpuTextureDesc& desc, const std::string& debugName = "") override;
    uint64_t CreateBuffer(const GpuBufferDesc& desc, const std::string& debugName = "") override;
    uint64_t CreateShader(const std::string& name, const std::string& source) override;
    uint64_t CreateShaderFromSpv(const std::string& name, const std::string& virtualPath) override;
    Ref<Shader> GetShader(uint64_t handle) override;
    uint64_t CreatePipeline(const void* pipelineDesc) override;
    uint64_t CreateGeometry(const void* geometryDesc) override;

    // ---- Resource Lookup ----
    GpuResourceSlot* GetTexture(uint64_t handle) override;
    GpuResourceSlot* GetBuffer(uint64_t handle) override;
    GpuResourceSlot* GetSlot(uint32_t index) override;

    // ---- External Reference Tracking ----
    void TrackExternalRef(uint64_t handle, const std::weak_ptr<void>& tracker) override;
    void SetGpuHandle(uint64_t handle, uint64_t gpuHandle);

    // ---- Resource Destruction ----
    void DestroyResource(uint64_t handle) override;

    // ---- Shader Loading ----
    uint64_t LoadShader(const std::string& virtualPath) override;
    const ShaderData* GetShaderData(uint64_t handle) const override;

    // ---- Frame Management ----
    void TickFrame() override;
    void Gc(uint32_t maxFramesInFlight) override;

    // ---- Queries ----
    size_t GetResourceCount() const override { return m_Slots.size() - m_FreeIndices.size(); }
    size_t GetSlotCount()    const { return m_Slots.size(); }

    static uint32_t GetIndex(uint64_t handle)      { return uint32_t(handle & 0xFFFFFFFF); }
    static uint16_t GetGeneration(uint64_t handle) { return uint16_t(handle >> 32); }
    static uint64_t MakeHandle(uint32_t index, uint16_t generation)
    {
        return (uint64_t(generation) << 32) | uint64_t(index);
    }

private:
    uint32_t AllocateSlot();
    bool     ValidateHandle(uint32_t index, uint16_t generation) const;

    // Shader loading helpers
    uint64_t AllocateShaderHandle();
    static std::string ReadFileToString(const std::filesystem::path& path);
    static void        CollectIncludeDirs(std::vector<std::filesystem::path>& outDirs,
                                          const std::filesystem::path& baseDir);
    static std::string ResolveIncludes(const std::string& source,
                                       const std::vector<std::filesystem::path>& includeDirs,
                                       std::unordered_set<std::string>& guardSet);

    FileSystem& m_FileSystem;

    std::vector<GpuResourceSlot> m_Slots;
    std::vector<uint32_t>        m_FreeIndices;
    uint32_t                     m_CurrentFrame = 0;
    std::unordered_map<uint32_t, std::weak_ptr<void>> m_ExternalTrackers;

    // Shader storage
    std::unordered_map<uint64_t, ShaderData> m_Shaders;
    std::unordered_map<uint64_t, Ref<Shader>> m_ShaderObjects;
    uint32_t          m_NextShaderIndex = 0;
    std::vector<uint32_t> m_FreeShaderIndices;
};

} // namespace Vulkitten
