#include "GLResources.hpp"

#include <GLFW/glfw3.h>

#include <cstring>
#include <cstdio>

namespace rhi {

// ============================================================
// GLBufferResource
// ============================================================

GLBufferResource::GLBufferResource(const BufferDesc& desc, const void* initialData)
    : m_Size(desc.Size)
    , m_Usage(desc.Usage)
{
    GLenum target = GL_ARRAY_BUFFER;
    GLenum usage = GL_STATIC_DRAW;

    if (desc.Usage == BufferUsage::Vertex || desc.Usage == BufferUsage::Storage)
    {
        target = GL_ARRAY_BUFFER;
        usage = GL_STATIC_DRAW;
    }
    else if (desc.Usage == BufferUsage::Index)
    {
        target = GL_ELEMENT_ARRAY_BUFFER;
        usage = GL_STATIC_DRAW;
    }
    else if (desc.Usage == BufferUsage::Uniform)
    {
        target = GL_UNIFORM_BUFFER;
        usage = GL_DYNAMIC_DRAW;
    }

    glGenBuffers(1, &m_GlBuffer);
    glBindBuffer(target, m_GlBuffer);
    glBufferData(target, static_cast<GLsizeiptr>(m_Size), initialData, usage);
}

GLBufferResource::~GLBufferResource()
{
    if (m_GlBuffer)
        glDeleteBuffers(1, &m_GlBuffer);
}

void* GLBufferResource::Map(uint64_t /*offset*/, uint64_t /*size*/)
{
    // [HACK: GL buffer mapping not implemented for MVP]
    return nullptr;
}

void GLBufferResource::Unmap() {}

void GLBufferResource::Flush(uint64_t, uint64_t) {}

// ============================================================
// GLTextureResource (stub)
// ============================================================

GLTextureResource::GLTextureResource(const TextureDesc& desc, const void* /*initialData*/)
    : m_Desc(desc)
{
}

GLTextureResource::~GLTextureResource() = default;

TextureType GLTextureResource::GetType() const { return m_Desc.Type; }
Format      GLTextureResource::GetFormat() const { return m_Desc.Format; }
Extent3D    GLTextureResource::GetExtent() const { return m_Desc.Extent; }
uint32_t    GLTextureResource::GetMipLevels() const { return m_Desc.MipLevels; }

// ============================================================
// GLShaderResource
// ============================================================

GLShaderResource::GLShaderResource(ShaderStage stage, const ShaderBytecode& bytecode)
    : m_Stage(stage)
{
    if (!bytecode.Data || bytecode.Size == 0)
    {
        fprintf(stderr, "GLShaderResource: empty bytecode\n");
        return;
    }

    // Check SPIR-V magic number (0x07230203)
    bool isSpirv = (bytecode.Size >= 4 &&
                    bytecode.Data[0] == 0x03 && bytecode.Data[1] == 0x02 &&
                    bytecode.Data[2] == 0x23 && bytecode.Data[3] == 0x07);

    GLenum glStage = 0;
    switch (stage)
    {
        case ShaderStage::Vertex:   glStage = GL_VERTEX_SHADER;   break;
        case ShaderStage::Fragment: glStage = GL_FRAGMENT_SHADER; break;
        case ShaderStage::Compute:  glStage = GL_COMPUTE_SHADER;  break;
        default:
            fprintf(stderr, "GLShaderResource: unsupported stage\n");
            return;
    }

    m_GlShader = glCreateShader(glStage);

    if (isSpirv)
    {
        using PFN_glSpecializeShader = void (GLAPIENTRY*)(GLuint, const GLchar*, GLuint,
                                                            const GLuint*, const GLuint*);
        auto glSpecializeShader = reinterpret_cast<PFN_glSpecializeShader>(
            glfwGetProcAddress("glSpecializeShader"));
        if (!glSpecializeShader)
            glSpecializeShader = reinterpret_cast<PFN_glSpecializeShader>(
                glfwGetProcAddress("glSpecializeShaderARB"));

        glShaderBinary(1, &m_GlShader, GL_SHADER_BINARY_FORMAT_SPIR_V,
                       bytecode.Data, static_cast<GLsizei>(bytecode.Size));

        if (glSpecializeShader)
        {
            glSpecializeShader(m_GlShader, bytecode.EntryPoint, 0, nullptr, nullptr);
        }
        else
        {
            fprintf(stderr, "GLShaderResource: glSpecializeShader not available\n");
        }

        GLint compiled = 0;
        glGetShaderiv(m_GlShader, GL_COMPILE_STATUS, &compiled);
        if (!compiled)
        {
            char log[1024];
            glGetShaderInfoLog(m_GlShader, sizeof(log), nullptr, log);
            fprintf(stderr, "GLShaderResource SPIR-V compile failed:\n%s\n", log);
            glDeleteShader(m_GlShader);
            m_GlShader = 0;
            return;
        }
    }
    else
    {
        // GLSL source
        const char* src = reinterpret_cast<const char*>(bytecode.Data);
        GLint len = static_cast<GLint>(bytecode.Size);
        glShaderSource(m_GlShader, 1, &src, &len);
        glCompileShader(m_GlShader);

        GLint compiled = 0;
        glGetShaderiv(m_GlShader, GL_COMPILE_STATUS, &compiled);
        if (!compiled)
        {
            char log[1024];
            glGetShaderInfoLog(m_GlShader, sizeof(log), nullptr, log);
            fprintf(stderr, "GLShaderResource GLSL compile failed:\n%s\n", log);
            glDeleteShader(m_GlShader);
            m_GlShader = 0;
            return;
        }
    }
}

GLShaderResource::~GLShaderResource()
{
    if (m_GlShader)
        glDeleteShader(m_GlShader);
}

// ============================================================
// GLPipelineResource
// ============================================================

GLPipelineResource::GLPipelineResource(const PipelineDesc& desc,
                                       GLShaderResource* vs,
                                       GLShaderResource* fs,
                                       GLShaderResource* cs)
    : m_IsCompute(desc.ComputeShader.IsValid())
    , m_VertexLayout(desc.VertexLayout)
    , m_Raster(desc.Raster)
    , m_DepthStencil(desc.DepthStencil)
    , m_BlendStates(desc.Blends)
    , m_TextureSlots(desc.TextureSlots)
    , m_BufferSlots(desc.BufferSlots)
    , m_PushConstantsSize(desc.PushConstantsSize)
{
    m_GlProgram = glCreateProgram();

    // Required for SPIR-V shaders that use separate_shader_objects
    glProgramParameteri(m_GlProgram, GL_PROGRAM_SEPARABLE, GL_TRUE);

    auto attachShader = [&](GLShaderResource* shader) {
        if (shader && shader->IsValid())
            glAttachShader(m_GlProgram, shader->GetGLShader());
    };

    attachShader(vs);
    attachShader(fs);
    attachShader(cs);

    glLinkProgram(m_GlProgram);

    GLint linked = 0;
    glGetProgramiv(m_GlProgram, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        char log[1024];
        glGetProgramInfoLog(m_GlProgram, sizeof(log), nullptr, log);
        fprintf(stderr, "GLPipelineResource link failed:\n%s\n", log);
        glDeleteProgram(m_GlProgram);
        m_GlProgram = 0;
        return;
    }

    // Validate to catch binding/interface issues early
    glValidateProgram(m_GlProgram);
    GLint valid = 0;
    glGetProgramiv(m_GlProgram, GL_VALIDATE_STATUS, &valid);
    if (!valid)
    {
        char log[1024];
        glGetProgramInfoLog(m_GlProgram, sizeof(log), nullptr, log);
        fprintf(stderr, "GLPipelineResource validation warning:\n%s\n", log);
    }
}

GLPipelineResource::~GLPipelineResource()
{
    // Clean up VAOs created for this pipeline
    for (auto& [geoId, vao] : m_VaoCache)
        glDeleteVertexArrays(1, &vao);
    m_VaoCache.clear();

    if (m_GlProgram)
        glDeleteProgram(m_GlProgram);
}

// ============================================================
// ApplyGLState — set ALL GL state from PSO
// ============================================================

static GLenum ToGLCullFace(RasterState::CullMode mode)
{
    switch (mode)
    {
        case RasterState::CullMode::None:  return GL_NONE;
        case RasterState::CullMode::Front: return GL_FRONT;
        case RasterState::CullMode::Back:  return GL_BACK;
        default: return GL_BACK;
    }
}

static GLenum ToGLFrontFace(RasterState::FrontFace ff)
{
    // Vulkan uses counter-clockwise = front by default
    // OpenGL uses counter-clockwise = front by default (matches)
    return (ff == RasterState::FrontFace::CounterClockwise) ? GL_CCW : GL_CW;
}

static GLenum ToGLCompareOp(DepthStencilState::CompareOp op)
{
    switch (op)
    {
        case DepthStencilState::CompareOp::Never:        return GL_NEVER;
        case DepthStencilState::CompareOp::Less:         return GL_LESS;
        case DepthStencilState::CompareOp::Equal:        return GL_EQUAL;
        case DepthStencilState::CompareOp::LessEqual:    return GL_LEQUAL;
        case DepthStencilState::CompareOp::Greater:      return GL_GREATER;
        case DepthStencilState::CompareOp::NotEqual:     return GL_NOTEQUAL;
        case DepthStencilState::CompareOp::GreaterEqual: return GL_GEQUAL;
        case DepthStencilState::CompareOp::Always:       return GL_ALWAYS;
        default: return GL_LESS;
    }
}

static GLenum ToGLPolygonMode(RasterState::PolygonMode mode)
{
    switch (mode)
    {
        case RasterState::PolygonMode::Fill:  return GL_FILL;
        case RasterState::PolygonMode::Line:  return GL_LINE;
        case RasterState::PolygonMode::Point: return GL_POINT;
        default: return GL_FILL;
    }
}

void GLPipelineResource::ApplyGLState() const
{
    if (!m_GlProgram) return;

    // ---- Program ----
    glUseProgram(m_GlProgram);

    // ---- Depth / Stencil ----
    if (m_DepthStencil.DepthTestEnable)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(ToGLCompareOp(m_DepthStencil.DepthCompareOp));
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }
    glDepthMask(m_DepthStencil.DepthWriteEnable ? GL_TRUE : GL_FALSE);

    if (m_DepthStencil.StencilTestEnable)
        glEnable(GL_STENCIL_TEST);
    else
        glDisable(GL_STENCIL_TEST);

    // ---- Cull Face ----
    GLenum cullMode = ToGLCullFace(m_Raster.Cull);
    if (cullMode == GL_NONE)
    {
        glDisable(GL_CULL_FACE);
    }
    else
    {
        glEnable(GL_CULL_FACE);
        glCullFace(cullMode);
    }
    glFrontFace(ToGLFrontFace(m_Raster.Front));

    // ---- Polygon Mode ----
    glPolygonMode(GL_FRONT_AND_BACK, ToGLPolygonMode(m_Raster.Poly));

    // ---- Blend ----
    if (!m_BlendStates.empty() && m_BlendStates[0].Enable)
    {
        glEnable(GL_BLEND);
        // [HACK: only first blend state applied; multi-RT not supported yet]
        const auto& b = m_BlendStates[0];
        // Use simple blend func — full blend factor mapping is complex
        // For MVP, standard alpha blending:
        //   srcAlpha = SrcAlpha, dstAlpha = OneMinusSrcAlpha
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else
    {
        glDisable(GL_BLEND);
    }

    // Color write mask from first blend state (or default all-on)
    if (!m_BlendStates.empty())
    {
        const auto& b = m_BlendStates[0];
        glColorMask(b.WriteR ? GL_TRUE : GL_FALSE,
                    b.WriteG ? GL_TRUE : GL_FALSE,
                    b.WriteB ? GL_TRUE : GL_FALSE,
                    b.WriteA ? GL_TRUE : GL_FALSE);
    }
}

// ============================================================
// GetOrCreateVAO — lazy VAO creation cached by geometry ID
// ============================================================

GLuint GLPipelineResource::GetOrCreateVAO(uint32_t geometryId,
                                          const std::vector<GLBufferResource*>& vertexBuffers,
                                          GLBufferResource* indexBuffer,
                                          IndexType indexType)
{
    auto it = m_VaoCache.find(geometryId);
    if (it != m_VaoCache.end())
        return it->second;

    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Bind index buffer
    if (indexBuffer && indexBuffer->GetGLBuffer())
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer->GetGLBuffer());
    }

    // Bind vertex buffers and set attributes
    for (size_t i = 0; i < m_VertexLayout.size(); ++i)
    {
        const auto& attr = m_VertexLayout[i];
        if (attr.BufferSlot >= vertexBuffers.size()) continue;

        auto* buf = vertexBuffers[attr.BufferSlot];
        if (!buf || !buf->GetGLBuffer()) continue;

        glBindBuffer(GL_ARRAY_BUFFER, buf->GetGLBuffer());

        GLint compCount = static_cast<GLint>(FormatComponentCount(attr.Format));
        glEnableVertexAttribArray(attr.Location);

        // Determine GL type from format
        GLenum glType = GL_FLOAT;
        Format fmt = attr.Format;
        if (fmt == Format::R32_UINT || fmt == Format::R32_SINT || fmt == Format::RG32_UINT || fmt == Format::RGBA32_UINT)
            glType = GL_UNSIGNED_INT;
        // else default to FLOAT for MVP

        glVertexAttribPointer(attr.Location, compCount, glType, GL_FALSE,
                              static_cast<GLsizei>(attr.Stride),
                              reinterpret_cast<void*>(static_cast<uintptr_t>(attr.Offset)));
    }

    m_VaoCache[geometryId] = vao;
    return vao;
}

// ============================================================
// GLGeometryResource
// ============================================================

GLGeometryResource::GLGeometryResource(const GeometryDesc& desc)
    : m_Desc(desc)
{
}

// ============================================================
// GLSamplerResource
// ============================================================

GLSamplerResource::GLSamplerResource(const SamplerDesc& desc)
{
    glGenSamplers(1, &m_GlSampler);

    auto toGLFilter = [](FilterMode f) -> GLenum {
        return (f == FilterMode::Nearest) ? GL_NEAREST : GL_LINEAR;
    };
    auto toGLWrap = [](WrapMode w) -> GLenum {
        switch (w)
        {
            case WrapMode::Repeat:         return GL_REPEAT;
            case WrapMode::ClampToEdge:    return GL_CLAMP_TO_EDGE;
            case WrapMode::ClampToBorder:  return GL_CLAMP_TO_BORDER;
            case WrapMode::MirroredRepeat: return GL_MIRRORED_REPEAT;
            default: return GL_REPEAT;
        }
    };

    glSamplerParameteri(m_GlSampler, GL_TEXTURE_MIN_FILTER,
        (desc.Mip == MipMode::Linear) ? GL_LINEAR_MIPMAP_LINEAR : static_cast<GLint>(toGLFilter(desc.MinFilter)));
    glSamplerParameteri(m_GlSampler, GL_TEXTURE_MAG_FILTER, toGLFilter(desc.MagFilter));
    glSamplerParameteri(m_GlSampler, GL_TEXTURE_WRAP_S, toGLWrap(desc.WrapU));
    glSamplerParameteri(m_GlSampler, GL_TEXTURE_WRAP_T, toGLWrap(desc.WrapV));
    glSamplerParameteri(m_GlSampler, GL_TEXTURE_WRAP_R, toGLWrap(desc.WrapW));
    glSamplerParameterf(m_GlSampler, GL_TEXTURE_MAX_ANISOTROPY, desc.MaxAnisotropy);
}

GLSamplerResource::~GLSamplerResource()
{
    if (m_GlSampler)
        glDeleteSamplers(1, &m_GlSampler);
}

// ============================================================
// GLRenderPassResource
// ============================================================

GLRenderPassResource::GLRenderPassResource(const RenderPassDesc& desc)
    : m_Desc(desc)
{
}

// ============================================================
// GLFramebufferResource
// ============================================================

GLFramebufferResource::GLFramebufferResource(const FramebufferDesc& desc,
                                             const std::vector<GLTextureResource*>& /*colorTexs*/,
                                             GLTextureResource* /*depthTex*/)
    : m_Desc(desc)
{
    if (!desc.ColorAttachments.empty() || desc.DepthStencilAttachment.IsValid())
    {
        glGenFramebuffers(1, &m_GlFbo);

        glBindFramebuffer(GL_FRAMEBUFFER, m_GlFbo);
        // [HACK: attachment binding omitted for MVP — textures are stubs]
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
            fprintf(stderr, "GLFramebufferResource: framebuffer incomplete (status=0x%x)\n", status);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    // else m_GlFbo stays 0 (default framebuffer)
}

GLFramebufferResource::~GLFramebufferResource()
{
    if (m_GlFbo)
        glDeleteFramebuffers(1, &m_GlFbo);
}

} // namespace rhi
