#pragma once

#include "Vulkitten/Renderer/IGpuResourceManager.h"

namespace Vulkitten {

class VulkanDevice;

// ============================================================
// VkGpuResourceManager - Vulkan GPU resource management.
//
// Implements IGpuResourceManager for Vulkan. Allocates VkBuffer
// and VkImage resources with proper memory binding and staging.
// Uses handle-based access (index + generation) safe for
// multi-frame-in-flight usage.
// ============================================================
class VkGpuResourceManager : public IGpuResourceManager
{
public:
    explicit VkGpuResourceManager(VulkanDevice& device);
    ~VkGpuResourceManager();

    // ---- IGpuResourceManager ----
    uint64_t CreateTexture(const GpuTextureDesc& desc, const std::string& debugName = "") override;
    uint64_t CreateBuffer(const GpuBufferDesc& desc, const std::string& debugName = "") override;
    uint64_t CreateShader(const std::string& name, const std::string& source) override;
    uint64_t CreatePipeline(const void* pipelineDesc) override;
    uint64_t CreateGeometry(const void* geometryDesc) override;
    uint64_t CreateShaderFromSpv(const std::string& name, const std::string& virtualPath) override;
    Ref<Shader> GetShader(uint64_t handle) override;

    uint64_t LoadShader(const std::string& virtualPath) override;
    const ShaderData* GetShaderData(uint64_t handle) const override;

    GpuResourceSlot* GetTexture(uint64_t handle) override;
    GpuResourceSlot* GetBuffer(uint64_t handle) override;
    GpuResourceSlot* GetSlot(uint32_t index) override;

    void TrackExternalRef(uint64_t handle, const std::weak_ptr<void>& tracker) override;
    void SetGpuHandle(uint64_t handle, uint64_t gpuHandle) override;
    void DestroyResource(uint64_t handle) override;

    void TickFrame() override;
    void Gc(uint32_t maxFramesInFlight) override;

    size_t GetResourceCount() const override;
    size_t GetSlotCount() const { return m_Slots.size(); }

    static uint32_t GetIndex(uint64_t handle)      { return uint32_t(handle & 0xFFFFFFFF); }
    static uint16_t GetGeneration(uint64_t handle) { return uint16_t(handle >> 32); }
    static uint64_t MakeHandle(uint32_t index, uint16_t gen)
    {
        return (uint64_t(gen) << 32) | uint64_t(index);
    }

private:
    uint32_t AllocateSlot();
    bool ValidateHandle(uint32_t index, uint16_t generation) const;

    VulkanDevice& m_Device;
    std::vector<GpuResourceSlot> m_Slots;
    std::vector<uint32_t>        m_FreeIndices;
    uint32_t                     m_CurrentFrame = 0;
    std::unordered_map<uint32_t, std::weak_ptr<void>> m_ExternalTrackers;
};

} // namespace Vulkitten
