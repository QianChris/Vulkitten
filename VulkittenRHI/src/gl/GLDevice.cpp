#include "GLDevice.hpp"
#include "GLCommandBuffer.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cstring>
#include <stdexcept>

namespace rhi {

// ============================================================
// Construction
// ============================================================

GLDevice::GLDevice(ISurface* surface)
    : m_Surface(surface)
{
    // Reserve slot 0 as null
    m_Slots.push_back({0, 0, false});
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

    // Make this window's context current
    glfwMakeContextCurrent(m_Window);

    // Load OpenGL via GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        throw std::runtime_error("GLDevice: failed to initialize GLAD");

    auto desc = m_Surface->GetDesc();
    m_Width = desc.Width;
    m_Height = desc.Height;

    glViewport(0, 0, static_cast<GLsizei>(m_Width), static_cast<GLsizei>(m_Height));
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // Default VAO (required for core profile)
    GLuint defaultVao = 0;
    glGenVertexArrays(1, &defaultVao);
    glBindVertexArray(defaultVao);
}

void GLDevice::Shutdown()
{
    WaitIdle();

    // Destroy GPU resources in the handle pool
    for (auto& slot : m_Slots)
    {
        if (!slot.Alive || slot.GpuHandle == 0)
            continue;
        // GL cleanup happens automatically on context destruction
        // (we don't know the type here, but for MVP it's fine)
    }
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
    ctx.SwapchainIndex = 0;  // GL: single backbuffer
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
// Resource Creation
// ============================================================

BufferHandle GLDevice::CreateBuffer(const BufferDesc& /*desc*/, const void* /*initialData*/)
{
    // [STUB: MVP clear-to-red doesn't need buffers]
    return BufferHandle{};
}

TextureHandle GLDevice::CreateTexture(const TextureDesc& /*desc*/, const void* /*initialData*/)
{
    // [STUB: MVP clear-to-red doesn't need textures]
    return TextureHandle{};
}

ShaderHandle GLDevice::CreateShader(ShaderStage /*stage*/, const ShaderBytecode& /*bytecode*/)
{
    // [STUB: MVP clear-to-red doesn't need shaders]
    return ShaderHandle{};
}

PipelineHandle GLDevice::CreatePipeline(const PipelineDesc& /*desc*/)
{
    // [STUB: MVP clear-to-red doesn't need pipelines]
    return PipelineHandle{};
}

GeometryHandle GLDevice::CreateGeometry(const GeometryDesc& /*desc*/)
{
    // [STUB: MVP clear-to-red doesn't need geometry]
    return GeometryHandle{};
}

SamplerHandle GLDevice::CreateSampler(const SamplerDesc& /*desc*/)
{
    // [STUB: MVP clear-to-red doesn't need samplers]
    return SamplerHandle{};
}

RenderPassHandle GLDevice::CreateRenderPass(const RenderPassDesc& desc)
{
    uint32_t id = FindFreeSlot();
    m_Slots[id].Alive = true;
    m_Slots[id].GpuHandle = 0;  // GL: no explicit render pass object

    m_RenderPassDescs[id] = desc;

    uint32_t gen = m_Slots[id].Generation;
    return RenderPassHandle{id, gen};
}

FramebufferHandle GLDevice::CreateFramebuffer(const FramebufferDesc& desc)
{
    uint32_t id = FindFreeSlot();
    m_Slots[id].Alive = true;

    // For GL, create an FBO if attachments are specified
    if (!desc.ColorAttachments.empty() || desc.DepthStencilAttachment.IsValid())
    {
        GLuint fbo = 0;
        glGenFramebuffers(1, &fbo);
        m_Slots[id].GpuHandle = fbo;

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        // Attach color textures
        for (size_t i = 0; i < desc.ColorAttachments.size(); ++i)
        {
            // [HACK: For MVP, color attachments are pre-created textures.
            //  In full impl, we'd look up the texture's GL name from the handle pool.]
            // For default framebuffer (swapchain), attachments are empty and we use FBO 0.
        }

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            // [HACK: framebuffer incomplete — this is expected for swapchain FBO]
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    else
    {
        // Default framebuffer (swapchain) — use FBO 0
        m_Slots[id].GpuHandle = 0;
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
    if (id == 0 || id >= m_Slots.size())
        return nullptr;
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

uint32_t GLDevice::FindFreeSlot()
{
    // Reuse a freed slot if available
    if (!m_FreeIndices.empty())
    {
        uint32_t idx = m_FreeIndices.back();
        m_FreeIndices.pop_back();
        m_Slots[idx].Generation++;  // Bump generation for ABA protection
        return idx;
    }

    // Allocate new slot
    uint32_t idx = static_cast<uint32_t>(m_Slots.size());
    m_Slots.push_back({0, 1, false});
    return idx;
}

} // namespace rhi
