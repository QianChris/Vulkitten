#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Renderer/Device.h"

namespace Vulkitten {

class VulkanInstance;

// ============================================================
// VulkanDevice — Vulkan implementation of IDevice.
//
// Handles physical device enumeration (preferring discrete GPU),
// logical device creation, and queue family selection (graphics,
// present, transfer). Resource creation methods are stubs until
// Task 15.
// ============================================================
class VulkanDevice : public IDevice
{
public:
    explicit VulkanDevice(VulkanInstance& instance);
    ~VulkanDevice();

    // ---- IDevice Lifecycle ----
    void Init() override;
    void Shutdown() override;

    // ---- IDevice Frame Lifecycle (stubs until Task 15) ----
    FrameContext beginFrame() override;
    void endFrame(FrameContext ctx) override;

    // ---- IDevice Command Buffer (stub until Task 16) ----
    rhi::ICommandBuffer* createCommandBuffer(FrameContext ctx) override;

    // ---- IDevice Resource Creation (stubs until Task 15) ----
    rhi::BufferHandle   createBuffer(const rhi::BufferDesc& desc, const void* initialData = nullptr) override;
    rhi::TextureHandle  createTexture(const rhi::TextureDesc& desc, const void* initialData = nullptr) override;
    rhi::ShaderHandle   createShader(rhi::ShaderStage stage, const ShaderBytecode& bytecode) override;
    rhi::PipelineHandle createPipeline(const rhi::PipelineDesc& desc) override;
    rhi::GeometryHandle createGeometry(const rhi::GeometryDesc& desc) override;
    rhi::SamplerHandle  createSampler(const rhi::SamplerDesc& desc) override;
    rhi::RenderPassHandle   createRenderPass(const rhi::RenderPassDesc& desc) override;
    rhi::FramebufferHandle  createFramebuffer(const rhi::FramebufferDesc& desc) override;

    // ---- IDevice Window ----
    void onResize(uint32_t width, uint32_t height) override;

    // ---- IDevice Utilities ----
    void waitIdle() override;
    void* getNativeDevice() const override { return m_NativeDevice; }

    // Deprecated PascalCase wrapper — use getNativeDevice() for IDevice interface.
    void* GetNativeDevice() const { return getNativeDevice(); }

    // ---- Queue Family Access ----
    uint32_t GetGraphicsQueueFamily() const { return m_GraphicsQueueFamily; }
    uint32_t GetPresentQueueFamily()  const { return m_PresentQueueFamily; }
    uint32_t GetTransferQueueFamily() const { return m_TransferQueueFamily; }

    void* GetNativePhysicalDevice() const { return m_PhysicalDevice; }

    // ---- Legacy ----
    void Submit(FrameContext& frameContext) override;

private:
    VulkanInstance& m_Instance;
    void* m_PhysicalDevice = nullptr;  // VkPhysicalDevice
    void* m_NativeDevice = nullptr;    // VkDevice
    uint32_t m_GraphicsQueueFamily = 0;
    uint32_t m_PresentQueueFamily = 0;
    uint32_t m_TransferQueueFamily = 0;
};

} // namespace Vulkitten
