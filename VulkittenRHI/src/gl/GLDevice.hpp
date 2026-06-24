#pragma once

#include "rhi/IRenderDevice.hpp"
#include "rhi/ISurface.hpp"

#include <vector>
#include <unordered_map>

struct GLFWwindow;

namespace rhi {

class GLCommandBuffer;

// ============================================================
// GLDevice — OpenGL implementation of IRenderDevice.
//
// For OpenGL, the GL context IS the device. Resource creation
// maps directly to GL calls. Handle allocation uses an internal
// slot-based pool with generation counters.
// ============================================================

class GLDevice : public IRenderDevice
{
public:
    explicit GLDevice(ISurface* surface);
    ~GLDevice() override;

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
    void* GetNativeDevice() override { return nullptr; }

    // ---- Internal Handle Pool (public for GLCommandBuffer) ----
    struct GpuSlot
    {
        uint64_t GpuHandle = 0;     // GLuint (texture/buffer/FBO)
        uint32_t Generation = 1;
        bool     Alive = false;
    };

    GpuSlot* GetSlot(uint32_t id);

    // ---- Metadata access (for GLCommandBuffer) ----
    const RenderPassDesc* GetRenderPassDesc(uint32_t id) const;
    const FramebufferDesc* GetFramebufferDesc(uint32_t id) const;

private:
    template<typename Tag>
    Handle<Tag> AllocHandle();
    uint32_t FindFreeSlot();

    ISurface*   m_Surface = nullptr;
    GLFWwindow* m_Window = nullptr;
    uint32_t    m_FrameIndex = 0;
    uint32_t    m_Width = 0;
    uint32_t    m_Height = 0;

    // Handle pool
    std::vector<GpuSlot> m_Slots;
    std::vector<uint32_t> m_FreeIndices;

    // Metadata storage (maps slot id → descriptor)
    std::unordered_map<uint32_t, RenderPassDesc> m_RenderPassDescs;
    std::unordered_map<uint32_t, FramebufferDesc> m_FramebufferDescs;
};

} // namespace rhi
