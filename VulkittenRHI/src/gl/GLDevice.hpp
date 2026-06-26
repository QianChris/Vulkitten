#pragma once

#include "rhi/IRenderDevice.hpp"
#include "rhi/ISurface.hpp"

#include <vector>
#include <unordered_map>
#include <memory>

struct GLFWwindow;

namespace rhi {

class ICommandBuffer;
class ResourceManager;
class GLCommandBuffer;
class GLBufferResource;
class GLTextureResource;
class GLShaderResource;
class GLPipelineResource;
class GLGeometryResource;
class GLSamplerResource;
class GLRenderPassResource;
class GLFramebufferResource;

// ============================================================
// GLDevice — OpenGL implementation of IRenderDevice
//
// Delegates handle allocation and resource storage to
// ResourceManager. Only creates native GL objects.
// ============================================================

class GLDevice : public IRenderDevice
{
public:
    explicit GLDevice(ISurface* surface, ResourceManager& rm);
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

    // ---- IRenderDevice Resource Query (delegates to ResourceManager) ----
    IBuffer*   GetBuffer(BufferHandle handle) override;
    ITexture*  GetTexture(TextureHandle handle) override;
    IShader*   GetShader(ShaderHandle handle) override;
    IPipeline* GetPipeline(PipelineHandle handle) override;
    IGeometry* GetGeometry(GeometryHandle handle) override;
    ISampler*  GetSampler(SamplerHandle handle) override;

    // ---- IRenderDevice Window Events ----
    void OnResize(uint32_t width, uint32_t height) override;
    void WaitIdle() override;
    void* GetNativeDevice() override { return nullptr; }

    // ---- ResourceManager access (for GLCommandBuffer) ----
    ResourceManager& GetResourceManager() { return m_Resources; }

    // ---- FBO cache (maps framebuffer handle ID → GLuint FBO) ----
    // [HACK: Framebuffer doesn't have a proper I* interface yet]
    void SetFbo(uint32_t id, uint32_t fbo) { m_FboMap[id] = fbo; }
    uint32_t GetFbo(uint32_t id) const {
        auto it = m_FboMap.find(id);
        return (it != m_FboMap.end()) ? it->second : 0;
    }

private:
    ISurface*   m_Surface = nullptr;
    GLFWwindow* m_Window = nullptr;
    uint32_t    m_FrameIndex = 0;
    uint32_t    m_Width = 0;
    uint32_t    m_Height = 0;

    ResourceManager& m_Resources;
    std::unordered_map<uint32_t, uint32_t> m_FboMap;
};

} // namespace rhi
