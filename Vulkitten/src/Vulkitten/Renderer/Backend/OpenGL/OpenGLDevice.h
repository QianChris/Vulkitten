#pragma once

#include "Vulkitten/Renderer/Device.h"

namespace Vulkitten {

// ============================================================
// OpenGLDevice — OpenGL implementation of IDevice.
//
// For OpenGL, the GL context IS the device — this is a thin
// wrapper. Resource creation methods are stubbed until Task 10.
// ============================================================
class VKT_API OpenGLDevice : public IDevice
{
public:
    explicit OpenGLDevice(void* nativeWindow = nullptr);

    // ---- IDevice Lifecycle ----
    void Init() override;
    void Shutdown() override;

    // ---- IDevice Frame Lifecycle ----
    FrameContext beginFrame() override;
    void endFrame(FrameContext ctx) override;

    // ---- IDevice Command Buffer ----
    ICommandBuffer* createCommandBuffer(FrameContext ctx) override;

    // ---- IDevice Resource Creation (stubs until Task 10) ----
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
    void* getNativeDevice() const override { return nullptr; }

    // ---- Legacy ----
    void Submit(FrameContext& frameContext) override;

private:
    void* m_NativeWindow = nullptr; // GLFWwindow*
};

} // namespace Vulkitten
