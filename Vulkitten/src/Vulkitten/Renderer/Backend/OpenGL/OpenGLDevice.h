#pragma once

#include "Vulkitten/Renderer/Device.h"

#include <vector>
#include <unordered_map>

namespace Vulkitten {

// ============================================================
// OpenGLDevice — OpenGL implementation of IDevice.
//
// For OpenGL, the GL context IS the device. Resource creation
// maps directly to GL calls (glGenBuffers, glGenTextures, etc.).
// Handle allocation uses an internal slot-based pool with
// generation counters for ABA protection.
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

    // ---- IDevice Command Buffer (stub until Task 12) ----
    ICommandBuffer* createCommandBuffer(FrameContext ctx) override;

    // ---- IDevice Resource Creation ----
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
    // ---- Internal Handle Pool ----
    struct GpuSlot
    {
        uint64_t GpuHandle = 0;     // GLuint / GL program handle
        uint32_t Generation = 1;    // ABA protection
        bool     Alive = false;
    };

    template<typename Tag>
    rhi::Handle<Tag> AllocHandle();
    GpuSlot* GetSlot(uint32_t id);
    uint32_t FindFreeSlot();

    void*              m_NativeWindow = nullptr;
    uint32_t           m_FrameIndex = 0;

    std::vector<GpuSlot> m_Slots;            // Handle pool
    std::vector<uint32_t> m_FreeIndices;     // Recycled slot indices
};

} // namespace Vulkitten
