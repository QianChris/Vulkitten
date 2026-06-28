#pragma once

#include "rhi/IBuffer.hpp"
#include "rhi/ITexture.hpp"
#include "rhi/IShader.hpp"
#include "rhi/IPipeline.hpp"
#include "rhi/IGeometry.hpp"
#include "rhi/ISampler.hpp"
#include "rhi/IRenderDevice.hpp"
#include "rhi/ResourceDescs.hpp"

#include <glad/glad.h>

#include <vector>
#include <unordered_map>
#include <memory>

namespace rhi {

class ResourceManager;

// ============================================================
// GLBufferResource — RAII GL buffer + IBuffer impl
// ============================================================
class GLBufferResource : public IBuffer
{
public:
    GLBufferResource(const BufferDesc& desc, const void* initialData);
    ~GLBufferResource() override;

    GLBufferResource(const GLBufferResource&) = delete;
    GLBufferResource& operator=(const GLBufferResource&) = delete;

    // IBuffer
    uint64_t GetSize() const override { return m_Size; }
    void*    Map(uint64_t offset, uint64_t size) override;
    void     Unmap() override;
    void     Flush(uint64_t offset, uint64_t size) override;

    // Native access
    GLuint     GetGLBuffer() const { return m_GlBuffer; }
    BufferUsage GetUsage() const { return m_Usage; }

private:
    GLuint     m_GlBuffer = 0;
    uint64_t   m_Size = 0;
    BufferUsage m_Usage = BufferUsage::None;
};

// ============================================================
// Format helpers (used by VAO and texture creation)
// ============================================================
GLenum FormatToGLInternal(Format f);
GLenum FormatToGLType(Format f, bool& outNormalized);

// ============================================================
// GLTextureResource — RAII GL texture
// ============================================================
class GLTextureResource : public ITexture
{
public:
    GLTextureResource(const TextureDesc& desc, const void* initialData);
    ~GLTextureResource() override;

    // ITexture
    TextureType GetType() const override;
    Format      GetFormat() const override;
    Extent3D    GetExtent() const override;
    uint32_t    GetMipLevels() const override;

    // Native access
    GLuint GetGLTexture() const { return m_GlTexture; }
    GLenum GetGLTarget() const;

private:
    void CreateTexture2D(const TextureDesc& desc, const void* initialData);

    GLuint     m_GlTexture = 0;
    TextureDesc m_Desc;
};

// ============================================================
// GLShaderResource — RAII GL shader object + IShader impl
// ============================================================
class GLShaderResource : public IShader
{
public:
    GLShaderResource(ShaderStage stage, const ShaderBytecode& bytecode);
    ~GLShaderResource() override;

    GLShaderResource(const GLShaderResource&) = delete;
    GLShaderResource& operator=(const GLShaderResource&) = delete;

    bool IsValid() const { return m_GlShader != 0; }

    // IShader
    ShaderStage GetStage() const override { return m_Stage; }
    const char* GetEntryPoint() const override { return "main"; }

    // Native access
    GLuint GetGLShader() const { return m_GlShader; }

private:
    GLuint      m_GlShader = 0;
    ShaderStage m_Stage;
};

// ============================================================
// GLPipelineResource — RAII GL program + FULL PSO state
//
// Stores the complete PipelineDesc so GLCommandBuffer can
// apply ALL GL state from the PSO during BindPipeline.
// ============================================================
class GLPipelineResource : public IPipeline
{
public:
    GLPipelineResource(const PipelineDesc& desc,
                       GLShaderResource* vs,
                       GLShaderResource* fs,
                       GLShaderResource* cs);
    ~GLPipelineResource() override;

    GLPipelineResource(const GLPipelineResource&) = delete;
    GLPipelineResource& operator=(const GLPipelineResource&) = delete;

    // IPipeline
    bool IsCompute() const override { return m_IsCompute; }

    // ---- Apply ALL GL state from this PSO ----
    // Called by GLCommandBuffer::BindPipeline.
    // Sets: program, depth test/write/func, cull face/front face,
    //       polygon mode, blend state, color mask.
    void ApplyGLState() const;

    // ---- Accessors ----
    GLuint GetGLProgram() const { return m_GlProgram; }
    const std::vector<VertexAttribute>& GetVertexLayout() const { return m_VertexLayout; }
    uint32_t GetPushConstantsSize() const { return m_PushConstantsSize; }
    const RasterState&      GetRasterState() const { return m_Raster; }
    const DepthStencilState& GetDepthStencil() const { return m_DepthStencil; }
    const std::vector<BlendState>& GetBlendStates() const { return m_BlendStates; }

    // ---- VAO cache (lazy, per pipeline+geometry pair) ----
    GLuint GetOrCreateVAO(uint32_t geometryId,
                          const std::vector<GLBufferResource*>& vertexBuffers,
                          GLBufferResource* indexBuffer,
                          IndexType indexType);

private:
    GLuint    m_GlProgram = 0;
    bool      m_IsCompute = false;

    // Full PSO state for GL state application
    std::vector<VertexAttribute> m_VertexLayout;
    RasterState       m_Raster;
    DepthStencilState m_DepthStencil;
    std::vector<BlendState> m_BlendStates;
    std::vector<TextureSlot> m_TextureSlots;
    std::vector<BufferSlot>  m_BufferSlots;
    uint32_t  m_PushConstantsSize = 0;

    // Lazy VAO cache: key = geometryId
    std::unordered_map<uint32_t, GLuint> m_VaoCache;
};

// ============================================================
// GLGeometryResource — geometry metadata + IGeometry impl
// ============================================================
class GLGeometryResource : public IGeometry
{
public:
    explicit GLGeometryResource(const GeometryDesc& desc);

    // IGeometry
    uint32_t GetVertexCount() const override { return m_Desc.VertexCount; }
    uint32_t GetIndexCount()   const override { return m_Desc.IndexCount; }

    const GeometryDesc& GetDesc() const { return m_Desc; }

private:
    GeometryDesc m_Desc;
};

// ============================================================
// GLSamplerResource — RAII GL sampler + ISampler impl
// ============================================================
class GLSamplerResource : public ISampler
{
public:
    explicit GLSamplerResource(const SamplerDesc& desc);
    ~GLSamplerResource() override;

    GLuint GetGLSampler() const { return m_GlSampler; }

private:
    GLuint m_GlSampler = 0;
};

// ============================================================
// GLRenderPassResource — stores RenderPassDesc
// ============================================================
class GLRenderPassResource
{
public:
    explicit GLRenderPassResource(const RenderPassDesc& desc);
    const RenderPassDesc& GetDesc() const { return m_Desc; }

private:
    RenderPassDesc m_Desc;
};

// ============================================================
// GLFramebufferResource — RAII GL FBO
// ============================================================
class GLFramebufferResource
{
public:
    GLFramebufferResource(const FramebufferDesc& desc,
                          const std::vector<GLTextureResource*>& colorTexs,
                          GLTextureResource* depthTex);
    ~GLFramebufferResource();

    GLFramebufferResource(const GLFramebufferResource&) = delete;
    GLFramebufferResource& operator=(const GLFramebufferResource&) = delete;

    GLuint GetGLFbo() const { return m_GlFbo; }
    const FramebufferDesc& GetDesc() const { return m_Desc; }

private:
    GLuint m_GlFbo = 0;
    FramebufferDesc m_Desc;
};

} // namespace rhi
