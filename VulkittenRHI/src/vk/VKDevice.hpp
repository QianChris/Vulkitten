#pragma once

#include "rhi/IRenderDevice.hpp"
#include "rhi/ISurface.hpp"
#include "rhi/ResourceDescs.hpp"
#include "VKSwapchain.hpp"

#include <vector>
#include <unordered_map>
#include <memory>

namespace rhi {

class VKCommandBuffer;
class ResourceManager;
class VKBufferResource;
class VKShaderResource;
class VKPipelineResource;

// ============================================================
// VKDevice — Vulkan implementation of IRenderDevice
//
// Delegates handle allocation and resource storage to
// ResourceManager. Only creates native Vulkan objects.
// ============================================================

class VKDevice : public IRenderDevice
{
public:
    explicit VKDevice(ISurface* surface, ResourceManager& rm);
    ~VKDevice() override;

    void Init() override;
    void Shutdown() override;

    FrameContext BeginFrame() override;
    void EndFrame(FrameContext ctx) override;

    std::unique_ptr<ICommandBuffer> CreateCommandBuffer(
        FrameContext ctx,
        CommandBufferLevel level = CommandBufferLevel::Primary) override;

    BufferHandle   CreateBuffer(const BufferDesc& desc, const void* initialData = nullptr) override;
    TextureHandle  CreateTexture(const TextureDesc& desc, const void* initialData = nullptr) override;
    ShaderHandle   CreateShader(ShaderStage stage, const ShaderBytecode& bytecode) override;
    PipelineHandle CreatePipeline(const PipelineDesc& desc) override;
    GeometryHandle CreateGeometry(const GeometryDesc& desc) override;
    SamplerHandle  CreateSampler(const SamplerDesc& desc) override;
    RenderPassHandle   CreateRenderPass(const RenderPassDesc& desc) override;
    FramebufferHandle  CreateFramebuffer(const FramebufferDesc& desc) override;

    IBuffer*   GetBuffer(BufferHandle handle) override;
    ITexture*  GetTexture(TextureHandle handle) override;
    IShader*   GetShader(ShaderHandle handle) override;
    IPipeline* GetPipeline(PipelineHandle handle) override;
    IGeometry* GetGeometry(GeometryHandle handle) override;
    ISampler*  GetSampler(SamplerHandle handle) override;

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
    void* GetRenderPass() const { return m_Swapchain ? m_Swapchain->GetRenderPass() : nullptr; }
    void* AllocateDescriptorSet(void* pipelineLayout);

    static constexpr uint64_t kSwapchainFramebufferSentinel = uint64_t(-1);

    // ---- ResourceManager access ----
    ResourceManager& GetResourceManager() { return m_Resources; }

private:
    uint32_t FindMemoryType(uint32_t typeFilter, uint32_t properties);
    void CreateCommandPools();
    void DestroyCommandPools();
    void CreateDescriptorPool();
    void DestroyDescriptorPool();

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

    std::vector<void*> m_CommandPools;
    void*              m_DescriptorPool = nullptr;

    uint32_t    m_FrameIndex = 0;
    uint32_t    m_CurrentImageIndex = 0;
    uint32_t    m_FramesInFlight = 2;
    bool        m_Initialized = false;
    void*       m_CurrentVkCmd = nullptr;

    // Framebuffer ID cache (maps handle ID → sentinel or native pointer)
    std::unordered_map<uint32_t, uint64_t> m_FramebufferHandles;

    ResourceManager& m_Resources;
};

} // namespace rhi
