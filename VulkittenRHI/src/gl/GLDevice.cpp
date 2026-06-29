#include "GLDevice.hpp"
#include "GLCommandBuffer.hpp"
#include "GLResources.hpp"
#include "rhi/ResourceManager.hpp"

#include <glad/glad.h>

#include <cstring>
#include <cstdio>
#include <stdexcept>

// Windows WGL — used directly instead of GLFW to avoid dual-linkage
// issues when GLFW is statically linked into both the DLL and the EXE.
#include <windows.h>

namespace rhi {

// ============================================================
// Construction
// ============================================================

GLDevice::GLDevice(ISurface* surface, ResourceManager& rm)
    : m_Surface(surface)
    , m_Resources(rm)
{
}

GLDevice::~GLDevice()
{
    Shutdown();
}

// ============================================================
// GL proc loader (same logic as glfwGetProcAddress, but uses
// the DLL's own WGL — no GLFW global state required.)
// ============================================================

static void* GlGetProcAddress(const char* name)
{
    void* proc = (void*)wglGetProcAddress((LPCSTR)name);
    if (proc)
        return proc;

    // Core OpenGL 1.0/1.1 functions live in opengl32.dll
    HMODULE module = GetModuleHandleA("opengl32.dll");
    if (module)
        return (void*)GetProcAddress(module, name);

    return nullptr;
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

    // Capture native WGL handles from the already-current context.
    // The caller (test fixture / renderer) must have made the GL
    // context current before calling Init().
    m_NativeDC  = (void*)wglGetCurrentDC();
    m_NativeHGLRC = (void*)wglGetCurrentContext();
    if (!m_NativeDC || !m_NativeHGLRC)
        throw std::runtime_error("GLDevice: no current GL context — "
                                 "call glfwMakeContextCurrent before Init()");

    if (!gladLoadGLLoader((GLADloadproc)GlGetProcAddress))
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
        fprintf(stderr, "GLDevice: GL_ARB_gl_spirv not supported - shader creation may fail\n");
}

void GLDevice::Shutdown()
{
    // Guard against double-call (destructor also calls Shutdown)
    if (!m_Window)
        return;

    // Make context current using native WGL (avoids GLFW cross-module issue)
    if (m_NativeDC && m_NativeHGLRC)
        wglMakeCurrent((HDC)m_NativeDC, (HGLRC)m_NativeHGLRC);
    WaitIdle();

    m_Window = nullptr;
    m_NativeDC = nullptr;
    m_NativeHGLRC = nullptr;
}

// ============================================================
// Frame Lifecycle
// ============================================================

FrameContext GLDevice::BeginFrame()
{
    // Use native WGL to make context current (avoids GLFW cross-module issue)
    if (m_NativeDC && m_NativeHGLRC)
        wglMakeCurrent((HDC)m_NativeDC, (HGLRC)m_NativeHGLRC);

    // Detect window resize and update viewport (matches VK backend behavior)
    auto surfDesc = m_Surface->GetDesc();
    if (surfDesc.Width != m_Width || surfDesc.Height != m_Height)
    {
        OnResize(surfDesc.Width, surfDesc.Height);
    }

    FrameContext ctx;
    ctx.FrameIndex = m_FrameIndex;
    ctx.SwapchainIndex = 0;
    return ctx;
}

void GLDevice::EndFrame(FrameContext /*ctx*/)
{
    if (m_NativeDC)
        SwapBuffers((HDC)m_NativeDC);
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
    uint32_t id = m_Resources.AllocateSlot();

    auto buffer = std::make_unique<GLBufferResource>(desc, initialData);

    m_Resources.StoreBuffer(id, std::move(buffer));

    uint32_t gen = m_Resources.GetGeneration(id);
    return BufferHandle{id, gen};
}

// ============================================================
// Texture (stub)
// ============================================================

TextureHandle GLDevice::CreateTexture(const TextureDesc& desc, const void* initialData)
{
    uint32_t id = m_Resources.AllocateSlot();

    auto texture = std::make_unique<GLTextureResource>(desc, initialData);

    m_Resources.StoreTexture(id, std::move(texture));

    uint32_t gen = m_Resources.GetGeneration(id);
    return TextureHandle{id, gen};
}

// ============================================================
// Shader
// ============================================================

ShaderHandle GLDevice::CreateShader(ShaderStage stage, const ShaderBytecode& bytecode)
{
    uint32_t id = m_Resources.AllocateSlot();

    auto shader = std::make_unique<GLShaderResource>(stage, bytecode);

    if (!shader->IsValid())
    {
        m_Resources.FreeSlot(id);
        return ShaderHandle{};
    }

    m_Resources.StoreShader(id, std::move(shader));

    uint32_t gen = m_Resources.GetGeneration(id);
    return ShaderHandle{id, gen};
}

// ============================================================
// Pipeline
// ============================================================

PipelineHandle GLDevice::CreatePipeline(const PipelineDesc& desc)
{
    // Resolve shader resources from ResourceManager
    auto* vs = dynamic_cast<GLShaderResource*>(m_Resources.GetShader(desc.VertexShader));
    auto* fs = dynamic_cast<GLShaderResource*>(m_Resources.GetShader(desc.FragmentShader));
    auto* cs = dynamic_cast<GLShaderResource*>(m_Resources.GetShader(desc.ComputeShader));

    uint32_t id = m_Resources.AllocateSlot();

    auto pipeline = std::make_unique<GLPipelineResource>(desc, vs, fs, cs);

    if (pipeline->GetGLProgram() == 0)
    {
        m_Resources.FreeSlot(id);
        return PipelineHandle{};
    }

    m_Resources.StorePipeline(id, std::move(pipeline));

    uint32_t gen = m_Resources.GetGeneration(id);
    return PipelineHandle{id, gen};
}

// ============================================================
// Geometry
// ============================================================

GeometryHandle GLDevice::CreateGeometry(const GeometryDesc& desc)
{
    uint32_t id = m_Resources.AllocateSlot();

    auto geometry = std::make_unique<GLGeometryResource>(desc);

    m_Resources.StoreGeometry(id, std::move(geometry));

    uint32_t gen = m_Resources.GetGeneration(id);
    return GeometryHandle{id, gen};
}

// ============================================================
// Sampler
// ============================================================

SamplerHandle GLDevice::CreateSampler(const SamplerDesc& desc)
{
    uint32_t id = m_Resources.AllocateSlot();

    auto sampler = std::make_unique<GLSamplerResource>(desc);

    m_Resources.StoreSampler(id, std::move(sampler));

    uint32_t gen = m_Resources.GetGeneration(id);
    return SamplerHandle{id, gen};
}

// ============================================================
// QueryPool
// ============================================================

QueryPoolHandle GLDevice::CreateQueryPool(const QueryPoolDesc& desc)
{
    uint32_t id = m_Resources.AllocateSlot();

    for (uint32_t i = 0; i < desc.Count; ++i)
    {
        GLuint query = 0;
        glGenQueries(1, &query);
        SetQueryGL(id, i, query);
    }

    uint32_t gen = m_Resources.GetGeneration(id);
    return QueryPoolHandle{id, gen};
}

// ============================================================
// RenderPass
// ============================================================

RenderPassHandle GLDevice::CreateRenderPass(const RenderPassDesc& desc)
{
    uint32_t id = m_Resources.AllocateSlot();
    m_Resources.StoreRenderPassDesc(id, desc);

    uint32_t gen = m_Resources.GetGeneration(id);
    return RenderPassHandle{id, gen};
}

// ============================================================
// Framebuffer
// ============================================================

FramebufferHandle GLDevice::CreateFramebuffer(const FramebufferDesc& desc)
{
    uint32_t id = m_Resources.AllocateSlot();

    // Collect texture resources for FBO attachment
    std::vector<GLTextureResource*> colorTexs;
    for (auto& h : desc.ColorAttachments)
    {
        auto* tex = dynamic_cast<GLTextureResource*>(m_Resources.GetTexture(h));
        colorTexs.push_back(tex);
    }
    auto* depthTex = dynamic_cast<GLTextureResource*>(m_Resources.GetTexture(desc.DepthStencilAttachment));

    auto fb = std::make_unique<GLFramebufferResource>(desc, colorTexs, depthTex);

    // Store FBO GLuint in member cache for GLCommandBuffer lookup
    SetFbo(id, fb->GetGLFbo());

    // Store framebuffer desc for render pass info
    m_Resources.StoreFramebufferDesc(id, desc);

    // [HACK: GLFramebufferResource RAII ownership lives in a static pool
    //  until Framebuffer gets a proper I* interface in ResourceManager.]
    static std::unordered_map<uint32_t, std::unique_ptr<GLFramebufferResource>> s_FboResCache;
    s_FboResCache[id] = std::move(fb);

    uint32_t gen = m_Resources.GetGeneration(id);
    return FramebufferHandle{id, gen};
}

// ============================================================
// Window Events
// ============================================================

void GLDevice::OnResize(uint32_t width, uint32_t height)
{
    m_Width = width;
    m_Height = height;
    if (m_NativeDC && m_NativeHGLRC)
    {
        wglMakeCurrent((HDC)m_NativeDC, (HGLRC)m_NativeHGLRC);
        glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
    }
}

void GLDevice::WaitIdle()
{
    glFinish();
}

// ============================================================
// Resource Query (delegates to ResourceManager)
// ============================================================

IBuffer*   GLDevice::GetBuffer(BufferHandle handle)     { return m_Resources.GetBuffer(handle); }
ITexture*  GLDevice::GetTexture(TextureHandle handle)    { return m_Resources.GetTexture(handle); }
IShader*   GLDevice::GetShader(ShaderHandle handle)      { return m_Resources.GetShader(handle); }
IPipeline* GLDevice::GetPipeline(PipelineHandle handle)  { return m_Resources.GetPipeline(handle); }
IGeometry* GLDevice::GetGeometry(GeometryHandle handle)  { return m_Resources.GetGeometry(handle); }
ISampler*  GLDevice::GetSampler(SamplerHandle handle)    { return m_Resources.GetSampler(handle); }

} // namespace rhi
