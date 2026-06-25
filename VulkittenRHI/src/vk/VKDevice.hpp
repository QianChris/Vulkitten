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
class IBuffer;
class ITexture;
class IShader;
class IPipeline;
class IGeometry;
class ISampler;

// Internal metadata for Vulkan resources
struct VKShaderMeta
{
    void* ShaderModule = nullptr;  // VkShaderModule
    ShaderStage Stage;
};

struct VKPipelineMeta
{
    void* PipelineLayout = nullptr;    // VkPipelineLayout
    void* Pipeline = nullptr;          // VkGraphicsPipeline
    void* DescriptorSetLayout = nullptr; // VkDescriptorSetLayout
    std::vector<VertexAttribute> VertexLayout;
    std::vector<BufferSlot> BufferSlots;
    std::vector<TextureSlot> TextureSlots;
};

struct VKBufferMeta
{
    void*   Buffer = nullptr;      // VkBuffer
    void*   Memory = nullptr;      // VkDeviceMemory
    uint64_t Size = 0;
    BufferUsage Usage;
};

struct VKGeometryMeta
{
    GeometryDesc Desc;
};

// ============================================================
// VKDevice — Vulkan implementation of IRenderDevice
// ============================================================

class VKDevice : public IRenderDevice
{
public:
    explicit VKDevice(ISurface* surface);
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

    // Metadata access
    const VKPipelineMeta* GetPipelineMeta(uint32_t id) const;
    const VKBufferMeta* GetBufferMeta(uint32_t id) const;
    const VKGeometryMeta* GetGeometryMeta(uint32_t id) const;

    // Descriptor set allocation (per-frame)
    void* AllocateDescriptorSet(void* pipelineLayout);

private:
    template<typename Tag> Handle<Tag> AllocHandle();
    uint32_t FindFreeSlot();
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

    std::vector<GpuSlot> m_Slots;
    std::vector<uint32_t> m_FreeIndices;

    std::unordered_map<uint32_t, VKShaderMeta> m_ShaderMetas;
    std::unordered_map<uint32_t, VKPipelineMeta> m_PipelineMetas;
    std::unordered_map<uint32_t, VKBufferMeta> m_BufferMetas;
    std::unordered_map<uint32_t, VKGeometryMeta> m_GeometryMetas;

    // Query interface cache (owned)
    std::unordered_map<uint32_t, std::unique_ptr<IBuffer>> m_BufferQueries;
    std::unordered_map<uint32_t, std::unique_ptr<ITexture>> m_TextureQueries;
    std::unordered_map<uint32_t, std::unique_ptr<IShader>> m_ShaderQueries;
    std::unordered_map<uint32_t, std::unique_ptr<IPipeline>> m_PipelineQueries;
    std::unordered_map<uint32_t, std::unique_ptr<IGeometry>> m_GeometryQueries;
    std::unordered_map<uint32_t, std::unique_ptr<ISampler>> m_SamplerQueries;

    uint32_t    m_FrameIndex = 0;
    uint32_t    m_CurrentImageIndex = 0;
    uint32_t    m_FramesInFlight = 2;
    bool        m_Initialized = false;
    void*       m_CurrentVkCmd = nullptr;
    uint32_t    m_DefaultRenderPassSlotId = 0;
};

} // namespace rhi
