#pragma once

#include "rhi/IRenderDevice.hpp"
#include "rhi/ISurface.hpp"

#include <vector>
#include <unordered_map>

struct GLFWwindow;

namespace rhi {

class GLCommandBuffer;
class IBuffer;
class ITexture;
class IShader;
class IPipeline;
class IGeometry;
class ISampler;

// Internal metadata for created resources
struct GLShaderMeta
{
    uint32_t GlShader = 0;       // GL shader object
    ShaderStage Stage;
};

struct GLPipelineMeta
{
    uint32_t GlProgram = 0;      // GL program object
    std::vector<VertexAttribute> VertexLayout;
    uint32_t PushConstantsSize = 0;
};

struct GLBufferMeta
{
    uint32_t GlBuffer = 0;       // GL buffer object
    uint64_t Size = 0;
    BufferUsage Usage;
};

// ============================================================
// GLDevice — OpenGL implementation of IRenderDevice.
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

    // ---- IRenderDevice Resource Query ----
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

    // ---- Internal Handle Pool (public for GLCommandBuffer) ----
    struct GpuSlot
    {
        uint64_t GpuHandle = 0;     // GLuint
        uint32_t Generation = 1;
        bool     Alive = false;
    };

    GpuSlot* GetSlot(uint32_t id);

    // Lazy VAO cache (key = (pipelineId << 32) | geometryId)
    uint32_t GetOrCreateVAO(uint64_t vaoKey, uint32_t pipelineId, uint32_t geometryId);

    // ---- Metadata access (for GLCommandBuffer) ----
    const RenderPassDesc* GetRenderPassDesc(uint32_t id) const;
    const FramebufferDesc* GetFramebufferDesc(uint32_t id) const;
    const GLPipelineMeta* GetPipelineMeta(uint32_t id) const;
    const GLBufferMeta* GetBufferMeta(uint32_t id) const;
    const GeometryDesc* GetGeometryDesc(uint32_t id) const;

private:
    template<typename Tag> Handle<Tag> AllocHandle();
    uint32_t FindFreeSlot();

    ISurface*   m_Surface = nullptr;
    GLFWwindow* m_Window = nullptr;
    uint32_t    m_FrameIndex = 0;
    uint32_t    m_Width = 0;
    uint32_t    m_Height = 0;

    // Handle pool
    std::vector<GpuSlot> m_Slots;
    std::vector<uint32_t> m_FreeIndices;

    // Metadata storage
    std::unordered_map<uint32_t, RenderPassDesc> m_RenderPassDescs;
    std::unordered_map<uint32_t, FramebufferDesc> m_FramebufferDescs;
    std::unordered_map<uint32_t, GLShaderMeta> m_ShaderMetas;
    std::unordered_map<uint32_t, GLPipelineMeta> m_PipelineMetas;
    std::unordered_map<uint32_t, GLBufferMeta> m_BufferMetas;
    std::unordered_map<uint32_t, GeometryDesc> m_GeometryDescs;

    // Lazy VAO cache (persistent across frames)
    std::unordered_map<uint64_t, uint32_t> m_VaoCache;

    // Query interface cache (owned)
    std::unordered_map<uint32_t, std::unique_ptr<IBuffer>> m_BufferQueries;
    std::unordered_map<uint32_t, std::unique_ptr<ITexture>> m_TextureQueries;
    std::unordered_map<uint32_t, std::unique_ptr<IShader>> m_ShaderQueries;
    std::unordered_map<uint32_t, std::unique_ptr<IPipeline>> m_PipelineQueries;
    std::unordered_map<uint32_t, std::unique_ptr<IGeometry>> m_GeometryQueries;
    std::unordered_map<uint32_t, std::unique_ptr<ISampler>> m_SamplerQueries;
};

} // namespace rhi
