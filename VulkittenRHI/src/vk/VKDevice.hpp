#pragma once

#include "rhi/IRenderDevice.hpp"
#include "rhi/ISurface.hpp"
#include "VKSwapchain.hpp"

#include <vector>
#include <unordered_map>
#include <memory>

namespace rhi {

class VKCommandBuffer;

// ============================================================
// VKDevice — Vulkan implementation of IRenderDevice
// ============================================================

class VKDevice : public IRenderDevice
{
public:
    explicit VKDevice(ISurface* surface);
    ~VKDevice() override;

    // ---- IRenderDevice Lifecycle ----
    void Init() override;
    void Shutdown() override;

    // ---- IRenderDevice Frame Lifecycle ----
    FrameContext BeginFrame() override;
    void EndFrame(FrameContext ctx) override;

    // ---- IRenderDevice Command Buffer ----
    std::unique_ptr<ICommandBuffer> CreateCommandBuffer(
        FrameContext ctx,
        CommandBufferLevel level = CommandBufferLevel::Primary) override;

    // ---- IRenderDevice Resource Creation ----
    BufferHandle   CreateBuffer(const BufferDesc& desc, const void* initialData = nullptr) override;
    TextureHandle  CreateTexture(const TextureDesc& desc, const void* initialData = nullptr) override;
    ShaderHandle   CreateShader(ShaderStage stage, const ShaderBytecode& bytecode) override;
    PipelineHandle CreatePipeline(const PipelineDesc& desc) override;
    GeometryHandle CreateGeometry(const GeometryDesc& desc) override;
    SamplerHandle  CreateSampler(const SamplerDesc& desc) override;
    RenderPassHandle   CreateRenderPass(const RenderPassDesc& desc) override;
    FramebufferHandle  CreateFramebuffer(const FramebufferDesc& desc) override;

    // ---- IRenderDevice Window Events ----
    void OnResize(uint32_t width, uint32_t height) override;
    void WaitIdle() override;
    void* GetNativeDevice() override { return m_Device; }

    // ---- Internal access (for VKCommandBuffer) ----
    void* GetVkDevice() const { return m_Device; }
    void* GetGraphicsQueue() const { return m_GraphicsQueue; }
    void* GetCommandPool(uint32_t frameIndex) const;
    uint32_t GetGraphicsQueueFamily() const { return m_GraphicsQueueFamily; }
    void* GetSwapchainFramebuffer(uint32_t imageIndex) const;
    uint32_t GetCurrentImageIndex() const { return m_CurrentImageIndex; }

    // Sentinel: marks a framebuffer handle as "swapchain-owned"
    static constexpr uint64_t kSwapchainFramebufferSentinel = uint64_t(-1);

    // Handle pool
    struct GpuSlot
    {
        uint64_t GpuHandle = 0;
        uint64_t GpuHandle2 = 0;
        uint32_t Generation = 1;
        bool     Alive = false;
    };
    GpuSlot* GetSlot(uint32_t id);

private:
    template<typename Tag> Handle<Tag> AllocHandle();
    uint32_t FindFreeSlot();
    uint32_t FindMemoryType(uint32_t typeFilter, uint32_t properties);
    void CreateCommandPools();
    void DestroyCommandPools();

    ISurface*   m_Surface = nullptr;
    void*       m_Instance = nullptr;
    void*       m_PhysicalDevice = nullptr;
    void*       m_Device = nullptr;
    void*       m_SurfaceVK = nullptr;
    void*       m_GraphicsQueue = nullptr;
    void*       m_PresentQueue = nullptr;
    uint32_t    m_GraphicsQueueFamily = 0;
    uint32_t    m_PresentQueueFamily = 0;

    std::unique_ptr<VKSwapchain> m_Swapchain;

    // Per-frame command pools
    std::vector<void*> m_CommandPools;

    // Handle pool
    std::vector<GpuSlot> m_Slots;
    std::vector<uint32_t> m_FreeIndices;

    uint32_t    m_FrameIndex = 0;
    uint32_t    m_CurrentImageIndex = 0;
    uint32_t    m_FramesInFlight = 2;
    bool        m_Initialized = false;
    void*       m_CurrentVkCmd = nullptr;     // VkCommandBuffer for current frame
    uint32_t    m_DefaultRenderPassSlotId = 0; // Slot for default swapchain RP
};

} // namespace rhi
