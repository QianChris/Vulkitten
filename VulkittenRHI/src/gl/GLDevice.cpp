#include "GLDevice.hpp"
#include "GLCommandBuffer.hpp"

#include "rhi/IBuffer.hpp"
#include "rhi/ITexture.hpp"
#include "rhi/IShader.hpp"
#include "rhi/IPipeline.hpp"
#include "rhi/IGeometry.hpp"
#include "rhi/ISampler.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cstring>
#include <cstdio>
#include <stdexcept>

namespace rhi {

// ============================================================
// Internal query interface implementations
// ============================================================

class GLBuffer final : public IBuffer
{
public:
    GLBuffer(const GLBufferMeta& meta) : m_Meta(meta) {}
    uint64_t GetSize() const override { return m_Meta.Size; }
    void* Map(uint64_t /*offset*/, uint64_t /*size*/) override { return nullptr; }
    void Unmap() override {}
    void Flush(uint64_t, uint64_t) override {}
private:
    const GLBufferMeta& m_Meta;
};

class GLTexture final : public ITexture
{
public:
    TextureType GetType() const override { return TextureType::Texture2D; }
    Format GetFormat() const override { return Format::Unknown; }
    Extent3D GetExtent() const override { return {}; }
    uint32_t GetMipLevels() const override { return 1; }
};

class GLShaderImpl final : public IShader
{
public:
    GLShaderImpl(const GLShaderMeta& meta) : m_Meta(meta) {}
    ShaderStage GetStage() const override { return m_Meta.Stage; }
    const char* GetEntryPoint() const override { return "main"; }
private:
    const GLShaderMeta& m_Meta;
};

class GLPipelineImpl final : public IPipeline
{
public:
    bool IsCompute() const override { return false; }
};

class GLGeometryImpl final : public IGeometry
{
public:
    GLGeometryImpl(const GeometryDesc& desc) : m_Desc(desc) {}
    uint32_t GetVertexCount() const override { return m_Desc.VertexCount; }
    uint32_t GetIndexCount() const override { return m_Desc.IndexCount; }
private:
    const GeometryDesc& m_Desc;
};

class GLSamplerImpl final : public ISampler {};

// ============================================================
// Construction
// ============================================================

GLDevice::GLDevice(ISurface* surface)
    : m_Surface(surface)
{
    m_Slots.push_back({0, 0, false});  // slot 0 = null
}

GLDevice::~GLDevice()
{
    Shutdown();
}

// ============================================================
// Init / Shutdown
// ============================================================

void GLDevice::Init()
{
    if (!m_Surface)
        throw std::runtime_error("GLDevice: no surface provided");

    m_Window = static_cast<GLFWwindow*>(m_Surface->GetNativeHandle());
    if (!m_Window)
        throw std::runtime_error("GLDevice: invalid native window handle");

    glfwMakeContextCurrent(m_Window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        throw std::runtime_error("GLDevice: failed to initialize GLAD");

    auto desc = m_Surface->GetDesc();
    m_Width = desc.Width;
    m_Height = desc.Height;

    glViewport(0, 0, static_cast<GLsizei>(m_Width), static_cast<GLsizei>(m_Height));

    // Default VAO (core profile requirement)
    GLuint defaultVao = 0;
    glGenVertexArrays(1, &defaultVao);
    glBindVertexArray(defaultVao);

    // Check SPIR-V support
    GLint numExts = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &numExts);
    bool hasSpirv = false;
    for (GLint i = 0; i < numExts; ++i)
    {
        const char* ext = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
        if (ext && strcmp(ext, "GL_ARB_gl_spirv") == 0)
        {
            hasSpirv = true;
            break;
        }
    }
    if (!hasSpirv)
        fprintf(stderr, "GLDevice: GL_ARB_gl_spirv not supported — shader creation may fail\n");
}

void GLDevice::Shutdown()
{
    WaitIdle();

    // Clean up VAOs
    for (auto& [key, vao] : m_VaoCache)
        glDeleteVertexArrays(1, &vao);
    m_VaoCache.clear();

    // Clean up GL objects
    for (auto& [id, meta] : m_ShaderMetas)
        if (meta.GlShader) glDeleteShader(meta.GlShader);
    for (auto& [id, meta] : m_PipelineMetas)
        if (meta.GlProgram) glDeleteProgram(meta.GlProgram);
    for (auto& [id, meta] : m_BufferMetas)
        if (meta.GlBuffer) glDeleteBuffers(1, &meta.GlBuffer);

    m_ShaderMetas.clear();
    m_PipelineMetas.clear();
    m_BufferMetas.clear();
    m_Slots.clear();
    m_FreeIndices.clear();
    m_Window = nullptr;
}

// ============================================================
// Frame Lifecycle
// ============================================================

FrameContext GLDevice::BeginFrame()
{
    if (m_Window)
        glfwMakeContextCurrent(m_Window);

    FrameContext ctx;
    ctx.FrameIndex = m_FrameIndex;
    ctx.SwapchainIndex = 0;
    return ctx;
}

void GLDevice::EndFrame(FrameContext /*ctx*/)
{
    if (m_Window)
        glfwSwapBuffers(m_Window);
    m_FrameIndex = (m_FrameIndex + 1) % 2;
}

// ============================================================
// Command Buffer
// ============================================================

std::unique_ptr<ICommandBuffer> GLDevice::CreateCommandBuffer(
    FrameContext /*ctx*/, CommandBufferLevel /*level*/)
{
    return std::make_unique<GLCommandBuffer>(*this);
}

// ============================================================
// Buffer
// ============================================================

BufferHandle GLDevice::CreateBuffer(const BufferDesc& desc, const void* initialData)
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

    GLuint glBuf = 0;
    glGenBuffers(1, &glBuf);
    glBindBuffer(target, glBuf);
    glBufferData(target, static_cast<GLsizeiptr>(desc.Size), initialData, usage);

    uint32_t id = FindFreeSlot();
    m_Slots[id].Alive = true;
    m_Slots[id].GpuHandle = static_cast<uint64_t>(glBuf);

    GLBufferMeta meta;
    meta.GlBuffer = glBuf;
    meta.Size = desc.Size;
    meta.Usage = desc.Usage;
    m_BufferMetas[id] = meta;

    uint32_t gen = m_Slots[id].Generation;
    return BufferHandle{id, gen};
}

// ============================================================
// Texture (stub)
// ============================================================

TextureHandle GLDevice::CreateTexture(const TextureDesc& /*desc*/, const void* /*initialData*/)
{
    return TextureHandle{};
}

// ============================================================
// Shader
// ============================================================

ShaderHandle GLDevice::CreateShader(ShaderStage stage, const ShaderBytecode& bytecode)
{
    if (!bytecode.Data || bytecode.Size == 0)
    {
        fprintf(stderr, "GLDevice::CreateShader: empty bytecode\n");
        return ShaderHandle{};
    }

    // Check SPIR-V magic number (0x07230203)
    bool isSpirv = (bytecode.Size >= 4 &&
                    bytecode.Data[0] == 0x03 && bytecode.Data[1] == 0x02 &&
                    bytecode.Data[2] == 0x23 && bytecode.Data[3] == 0x07);

    GLuint glShader = 0;
    GLenum glStage = 0;

    switch (stage)
    {
        case ShaderStage::Vertex:   glStage = GL_VERTEX_SHADER;   break;
        case ShaderStage::Fragment: glStage = GL_FRAGMENT_SHADER; break;
        case ShaderStage::Compute:  glStage = GL_COMPUTE_SHADER;  break;
        default:
            fprintf(stderr, "GLDevice::CreateShader: unsupported stage\n");
            return ShaderHandle{};
    }

    glShader = glCreateShader(glStage);

    if (isSpirv)
    {
        // Load glSpecializeShader — try core name first, then ARB fallback
        using PFN_glSpecializeShader = void (GLAPIENTRY*)(GLuint, const GLchar*, GLuint,
                                                            const GLuint*, const GLuint*);
        auto glSpecializeShader = reinterpret_cast<PFN_glSpecializeShader>(
            glfwGetProcAddress("glSpecializeShader"));
        if (!glSpecializeShader)
            glSpecializeShader = reinterpret_cast<PFN_glSpecializeShader>(
                glfwGetProcAddress("glSpecializeShaderARB"));

        glShaderBinary(1, &glShader, GL_SHADER_BINARY_FORMAT_SPIR_V,
                       bytecode.Data, static_cast<GLsizei>(bytecode.Size));

        if (glSpecializeShader)
        {
            glSpecializeShader(glShader, bytecode.EntryPoint, 0, nullptr, nullptr);
        }
        else
        {
            fprintf(stderr, "GLDevice: glSpecializeShader not available — SPIR-V won't work\n");
        }

        GLint compiled = 0;
        glGetShaderiv(glShader, GL_COMPILE_STATUS, &compiled);
        if (!compiled)
        {
            char log[1024];
            glGetShaderInfoLog(glShader, sizeof(log), nullptr, log);
            fprintf(stderr, "GLDevice::CreateShader SPIR-V compile failed:\n%s\n", log);
            glDeleteShader(glShader);
            return ShaderHandle{};
        }
    }
    else
    {
        // GLSL source
        const char* src = reinterpret_cast<const char*>(bytecode.Data);
        GLint len = static_cast<GLint>(bytecode.Size);
        glShaderSource(glShader, 1, &src, &len);
        glCompileShader(glShader);

        GLint compiled = 0;
        glGetShaderiv(glShader, GL_COMPILE_STATUS, &compiled);
        if (!compiled)
        {
            char log[1024];
            glGetShaderInfoLog(glShader, sizeof(log), nullptr, log);
            fprintf(stderr, "GLDevice::CreateShader GLSL compile failed:\n%s\n", log);
            glDeleteShader(glShader);
            return ShaderHandle{};
        }
    }

    uint32_t id = FindFreeSlot();
    m_Slots[id].Alive = true;
    m_Slots[id].GpuHandle = static_cast<uint64_t>(glShader);

    GLShaderMeta meta;
    meta.GlShader = glShader;
    meta.Stage = stage;
    m_ShaderMetas[id] = meta;

    uint32_t gen = m_Slots[id].Generation;
    return ShaderHandle{id, gen};
}

// ============================================================
// Pipeline
// ============================================================

PipelineHandle GLDevice::CreatePipeline(const PipelineDesc& desc)
{
    GLuint program = glCreateProgram();

    // Required for SPIR-V shaders that use separate_shader_objects
    glProgramParameteri(program, GL_PROGRAM_SEPARABLE, GL_TRUE);

    // Attach shaders
    auto attachShader = [&](ShaderHandle handle) {
        if (!handle.IsValid()) return;
        auto it = m_ShaderMetas.find(handle.GetId());
        if (it != m_ShaderMetas.end() && it->second.GlShader)
            glAttachShader(program, it->second.GlShader);
    };

    attachShader(desc.VertexShader);
    attachShader(desc.FragmentShader);
    attachShader(desc.ComputeShader);

    glLinkProgram(program);

    GLint linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        char log[1024];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        fprintf(stderr, "GLDevice::CreatePipeline link failed:\n%s\n", log);
        glDeleteProgram(program);
        return PipelineHandle{};
    }

    // Validate program to catch binding/interface issues early
    glValidateProgram(program);
    GLint valid = 0;
    glGetProgramiv(program, GL_VALIDATE_STATUS, &valid);
    if (!valid)
    {
        char log[1024];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        fprintf(stderr, "GLDevice::CreatePipeline validation warning:\n%s\n", log);
    }

    uint32_t id = FindFreeSlot();
    m_Slots[id].Alive = true;
    m_Slots[id].GpuHandle = static_cast<uint64_t>(program);

    GLPipelineMeta meta;
    meta.GlProgram = program;
    meta.VertexLayout = desc.VertexLayout;
    meta.PushConstantsSize = desc.PushConstantsSize;
    m_PipelineMetas[id] = meta;

    uint32_t gen = m_Slots[id].Generation;
    return PipelineHandle{id, gen};
}

// ============================================================
// Geometry
// ============================================================

GeometryHandle GLDevice::CreateGeometry(const GeometryDesc& desc)
{
    uint32_t id = FindFreeSlot();
    m_Slots[id].Alive = true;
    m_Slots[id].GpuHandle = 0;  // no GLObj — VAO is created lazily at bind time

    m_GeometryDescs[id] = desc;

    uint32_t gen = m_Slots[id].Generation;
    return GeometryHandle{id, gen};
}

const GeometryDesc* GLDevice::GetGeometryDesc(uint32_t id) const
{
    auto it = m_GeometryDescs.find(id);
    return (it != m_GeometryDescs.end()) ? &it->second : nullptr;
}

// ============================================================
// Sampler (stub)
// ============================================================

SamplerHandle GLDevice::CreateSampler(const SamplerDesc& /*desc*/)
{
    return SamplerHandle{};
}

// ============================================================
// RenderPass
// ============================================================

RenderPassHandle GLDevice::CreateRenderPass(const RenderPassDesc& desc)
{
    uint32_t id = FindFreeSlot();
    m_Slots[id].Alive = true;
    m_Slots[id].GpuHandle = 0;
    m_RenderPassDescs[id] = desc;
    uint32_t gen = m_Slots[id].Generation;
    return RenderPassHandle{id, gen};
}

// ============================================================
// Framebuffer
// ============================================================

FramebufferHandle GLDevice::CreateFramebuffer(const FramebufferDesc& desc)
{
    uint32_t id = FindFreeSlot();
    m_Slots[id].Alive = true;

    if (!desc.ColorAttachments.empty() || desc.DepthStencilAttachment.IsValid())
    {
        GLuint fbo = 0;
        glGenFramebuffers(1, &fbo);
        m_Slots[id].GpuHandle = fbo;

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        // [HACK: attachment binding omitted for MVP]
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
            fprintf(stderr, "GLDevice: framebuffer incomplete (status=0x%x)\n", status);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    else
    {
        m_Slots[id].GpuHandle = 0;  // default framebuffer
    }

    m_FramebufferDescs[id] = desc;
    uint32_t gen = m_Slots[id].Generation;
    return FramebufferHandle{id, gen};
}

// ============================================================
// Window Events
// ============================================================

void GLDevice::OnResize(uint32_t width, uint32_t height)
{
    m_Width = width;
    m_Height = height;
    if (m_Window)
    {
        glfwMakeContextCurrent(m_Window);
        glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
    }
}

void GLDevice::WaitIdle()
{
    glFinish();
}

// ============================================================
// Internal: Handle Pool
// ============================================================

GLDevice::GpuSlot* GLDevice::GetSlot(uint32_t id)
{
    if (id == 0 || id >= m_Slots.size()) return nullptr;
    return &m_Slots[id];
}

const RenderPassDesc* GLDevice::GetRenderPassDesc(uint32_t id) const
{
    auto it = m_RenderPassDescs.find(id);
    return (it != m_RenderPassDescs.end()) ? &it->second : nullptr;
}

const FramebufferDesc* GLDevice::GetFramebufferDesc(uint32_t id) const
{
    auto it = m_FramebufferDescs.find(id);
    return (it != m_FramebufferDescs.end()) ? &it->second : nullptr;
}

const GLPipelineMeta* GLDevice::GetPipelineMeta(uint32_t id) const
{
    auto it = m_PipelineMetas.find(id);
    return (it != m_PipelineMetas.end()) ? &it->second : nullptr;
}

const GLBufferMeta* GLDevice::GetBufferMeta(uint32_t id) const
{
    auto it = m_BufferMetas.find(id);
    return (it != m_BufferMetas.end()) ? &it->second : nullptr;
}

uint32_t GLDevice::GetOrCreateVAO(uint64_t vaoKey, uint32_t pipelineId, uint32_t geometryId)
{
    auto it = m_VaoCache.find(vaoKey);
    if (it != m_VaoCache.end())
        return it->second;

    auto* pipelineMeta = GetPipelineMeta(pipelineId);
    auto* geoDesc = GetGeometryDesc(geometryId);
    if (!pipelineMeta || !geoDesc)
        return 0;

    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    const auto& vtxLayout = pipelineMeta->VertexLayout;

    if (geoDesc->IndexBuffer.IsValid())
    {
        auto* ibMeta = GetBufferMeta(geoDesc->IndexBuffer.GetId());
        if (ibMeta)
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibMeta->GlBuffer);
    }

    for (const auto& attr : vtxLayout)
    {
        if (attr.BufferSlot >= geoDesc->VertexBufferCount) continue;
        BufferHandle bufHandle = geoDesc->VertexBuffers[attr.BufferSlot];
        auto* bufMeta = GetBufferMeta(bufHandle.GetId());
        if (!bufMeta) continue;

        glBindBuffer(GL_ARRAY_BUFFER, bufMeta->GlBuffer);

        GLint compCount = static_cast<GLint>(FormatComponentCount(attr.Format));
        glEnableVertexAttribArray(attr.Location);
        glVertexAttribPointer(attr.Location, compCount, GL_FLOAT, GL_FALSE,
                              static_cast<GLsizei>(attr.Stride),
                              reinterpret_cast<void*>(static_cast<uintptr_t>(attr.Offset)));
    }

    m_VaoCache[vaoKey] = vao;
    return vao;
}

uint32_t GLDevice::FindFreeSlot()
{
    if (!m_FreeIndices.empty())
    {
        uint32_t idx = m_FreeIndices.back();
        m_FreeIndices.pop_back();
        m_Slots[idx].Generation++;
        return idx;
    }
    uint32_t idx = static_cast<uint32_t>(m_Slots.size());
    m_Slots.push_back({0, 1, false});
    return idx;
}

// ============================================================
// Resource Query
// ============================================================

IBuffer* GLDevice::GetBuffer(BufferHandle handle)
{
    if (!handle.IsValid()) return nullptr;
    auto it = m_BufferQueries.find(handle.GetId());
    if (it != m_BufferQueries.end()) return it->second.get();
    auto metaIt = m_BufferMetas.find(handle.GetId());
    if (metaIt == m_BufferMetas.end()) return nullptr;
    auto ptr = std::make_unique<GLBuffer>(metaIt->second);
    IBuffer* raw = ptr.get();
    m_BufferQueries[handle.GetId()] = std::move(ptr);
    return raw;
}

ITexture* GLDevice::GetTexture(TextureHandle handle)
{
    if (!handle.IsValid()) return nullptr;
    auto it = m_TextureQueries.find(handle.GetId());
    if (it != m_TextureQueries.end()) return it->second.get();
    auto ptr = std::make_unique<GLTexture>();
    ITexture* raw = ptr.get();
    m_TextureQueries[handle.GetId()] = std::move(ptr);
    return raw;
}

IShader* GLDevice::GetShader(ShaderHandle handle)
{
    if (!handle.IsValid()) return nullptr;
    auto it = m_ShaderQueries.find(handle.GetId());
    if (it != m_ShaderQueries.end()) return it->second.get();
    auto metaIt = m_ShaderMetas.find(handle.GetId());
    if (metaIt == m_ShaderMetas.end()) return nullptr;
    auto ptr = std::make_unique<GLShaderImpl>(metaIt->second);
    IShader* raw = ptr.get();
    m_ShaderQueries[handle.GetId()] = std::move(ptr);
    return raw;
}

IPipeline* GLDevice::GetPipeline(PipelineHandle handle)
{
    if (!handle.IsValid()) return nullptr;
    auto it = m_PipelineQueries.find(handle.GetId());
    if (it != m_PipelineQueries.end()) return it->second.get();
    auto ptr = std::make_unique<GLPipelineImpl>();
    IPipeline* raw = ptr.get();
    m_PipelineQueries[handle.GetId()] = std::move(ptr);
    return raw;
}

IGeometry* GLDevice::GetGeometry(GeometryHandle handle)
{
    if (!handle.IsValid()) return nullptr;
    auto it = m_GeometryQueries.find(handle.GetId());
    if (it != m_GeometryQueries.end()) return it->second.get();
    auto metaIt = m_GeometryDescs.find(handle.GetId());
    if (metaIt == m_GeometryDescs.end()) return nullptr;
    auto ptr = std::make_unique<GLGeometryImpl>(metaIt->second);
    IGeometry* raw = ptr.get();
    m_GeometryQueries[handle.GetId()] = std::move(ptr);
    return raw;
}

ISampler* GLDevice::GetSampler(SamplerHandle handle)
{
    if (!handle.IsValid()) return nullptr;
    auto it = m_SamplerQueries.find(handle.GetId());
    if (it != m_SamplerQueries.end()) return it->second.get();
    auto ptr = std::make_unique<GLSamplerImpl>();
    ISampler* raw = ptr.get();
    m_SamplerQueries[handle.GetId()] = std::move(ptr);
    return raw;
}

} // namespace rhi
