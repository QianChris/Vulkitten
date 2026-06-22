#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Renderer/Device.h"

#include <vector>

namespace Vulkitten {

class VulkanInstance;

class VulkanDevice : public IDevice
{
public:
    explicit VulkanDevice(VulkanInstance& instance);
    ~VulkanDevice();

    void Init() override;
    void Shutdown() override;

    FrameContext beginFrame() override;
    void endFrame(FrameContext ctx) override;

    rhi::ICommandBuffer* createCommandBuffer(FrameContext ctx) override;

    rhi::BufferHandle   createBuffer(const rhi::BufferDesc& desc, const void* initialData = nullptr) override;
    rhi::TextureHandle  createTexture(const rhi::TextureDesc& desc, const void* initialData = nullptr) override;
    rhi::ShaderHandle   createShader(rhi::ShaderStage stage, const ShaderBytecode& bytecode) override;
    rhi::PipelineHandle createPipeline(const rhi::PipelineDesc& desc) override;
    rhi::GeometryHandle createGeometry(const rhi::GeometryDesc& desc) override;
    rhi::SamplerHandle  createSampler(const rhi::SamplerDesc& desc) override;
    rhi::RenderPassHandle   createRenderPass(const rhi::RenderPassDesc& desc) override;
    rhi::FramebufferHandle  createFramebuffer(const rhi::FramebufferDesc& desc) override;

    void onResize(uint32_t width, uint32_t height) override;
    void waitIdle() override;
    void* getNativeDevice() const override { return m_NativeDevice; }
    void* GetNativeDevice() const { return getNativeDevice(); }

    uint32_t GetGraphicsQueueFamily() const { return m_GraphicsQueueFamily; }
    uint32_t GetPresentQueueFamily()  const { return m_PresentQueueFamily; }
    uint32_t GetTransferQueueFamily() const { return m_TransferQueueFamily; }
    void* GetNativePhysicalDevice() const { return m_PhysicalDevice; }

    void Submit(FrameContext& frameContext) override;

    // Handle pool (public for VkCommandBuffer access)
    struct GpuSlot
    {
        uint64_t GpuHandle = 0;
        uint64_t GpuHandle2 = 0; // For VkImageView alongside VkImage
        uint32_t Generation = 1;
        bool     Alive = false;
    };
    GpuSlot* GetSlot(uint32_t id);

private:
    template<typename Tag> rhi::Handle<Tag> AllocHandle();
    uint32_t FindFreeSlot();
    uint32_t FindMemoryType(uint32_t typeFilter, uint32_t properties);

    VulkanInstance& m_Instance;
    void* m_PhysicalDevice = nullptr;
    void* m_NativeDevice = nullptr;
    uint32_t m_GraphicsQueueFamily = 0;
    uint32_t m_PresentQueueFamily = 0;
    uint32_t m_TransferQueueFamily = 0;

    std::vector<GpuSlot> m_Slots;
    std::vector<uint32_t> m_FreeIndices;
};

} // namespace Vulkitten
