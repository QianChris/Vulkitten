#include "vktpch.h"
#include "OpenGLDevice.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "Vulkitten/Renderer/FrameContext.h"
#include "Vulkitten/RHI/ICommandBuffer.hpp"
#include "Vulkitten/Renderer/Backend/OpenGL/GLCommandBuffer.h"
#include "Vulkitten/Perf/Instrumentor.h"

#include <cstring>

namespace Vulkitten {

// ============================================================
// Handle Pool Helpers
// ============================================================

template<typename Tag>
rhi::Handle<Tag> OpenGLDevice::AllocHandle()
{
    uint32_t id = FindFreeSlot();
    GpuSlot& slot = m_Slots[id];
    slot.Alive = true;
    return rhi::Handle<Tag>(id, slot.Generation);
}

OpenGLDevice::GpuSlot* OpenGLDevice::GetSlot(uint32_t id)
{
    if (id >= m_Slots.size())
        return nullptr;
    return &m_Slots[id];
}

uint32_t OpenGLDevice::FindFreeSlot()
{
    if (!m_FreeIndices.empty())
    {
        uint32_t id = m_FreeIndices.back();
        m_FreeIndices.pop_back();
        m_Slots[id].Generation++;
        m_Slots[id].GpuHandle = 0;
        return id;
    }
    m_Slots.emplace_back();
    return static_cast<uint32_t>(m_Slots.size() - 1);
}

// ============================================================
// Lifecycle
// ============================================================

OpenGLDevice::OpenGLDevice(void* nativeWindow)
    : m_NativeWindow(nativeWindow)
{
    // Reserve slot 0 as null/invalid
    m_Slots.emplace_back();
}

void OpenGLDevice::Init()
{
    VKT_PROFILE_RENDER_FUNCTION();
}

void OpenGLDevice::Shutdown()
{
    VKT_PROFILE_RENDER_FUNCTION();
}

// ============================================================
// Frame Lifecycle
// ============================================================

FrameContext OpenGLDevice::beginFrame()
{
    FrameContext ctx;
    ctx.FrameIndex = m_FrameIndex;
    m_FrameIndex = (m_FrameIndex + 1) % 3;  // Ring buffer of 3
    return ctx;
}

void OpenGLDevice::endFrame(FrameContext /*ctx*/)
{
    if (m_NativeWindow)
        glfwSwapBuffers(static_cast<GLFWwindow*>(m_NativeWindow));
}

// ============================================================
// Command Buffer (stub until Task 12)
// ============================================================

rhi::ICommandBuffer* OpenGLDevice::createCommandBuffer(FrameContext /*ctx*/)
{
    return new GLCommandBuffer(*this);
}

// ============================================================
// Buffer Creation
// ============================================================

rhi::BufferHandle OpenGLDevice::createBuffer(const rhi::BufferDesc& desc, const void* initialData)
{
    GLuint glBuf = 0;
    glGenBuffers(1, &glBuf);
    if (!glBuf)
        return {};

    GLenum target = GL_ARRAY_BUFFER;
    if (rhi::HasUsage(desc.Usage, rhi::BufferUsage::Index))
        target = GL_ELEMENT_ARRAY_BUFFER;
    else if (rhi::HasUsage(desc.Usage, rhi::BufferUsage::Uniform))
        target = GL_UNIFORM_BUFFER;
    else if (rhi::HasUsage(desc.Usage, rhi::BufferUsage::Storage))
        target = GL_SHADER_STORAGE_BUFFER;

    glBindBuffer(target, glBuf);
    glBufferData(target, desc.Size, initialData, GL_DYNAMIC_DRAW);
    glBindBuffer(target, 0);

    auto handle = AllocHandle<rhi::BufferTag>();
    GpuSlot* slot = GetSlot(handle.GetId());
    if (slot)
        slot->GpuHandle = static_cast<uint64_t>(glBuf);

    return handle;
}

// ============================================================
// Texture Creation
// ============================================================

static GLenum ToGLFormat(rhi::Format fmt)
{
    switch (fmt)
    {
        case rhi::Format::R8_UNORM:          return GL_R8;
        case rhi::Format::RG8_UNORM:         return GL_RG8;
        case rhi::Format::RGBA8_UNORM:       return GL_RGBA8;
        case rhi::Format::BGRA8_UNORM:       return GL_BGRA;
        case rhi::Format::RGBA8_SRGB:        return GL_SRGB8_ALPHA8;
        case rhi::Format::R32_FLOAT:         return GL_R32F;
        case rhi::Format::RGBA32_FLOAT:      return GL_RGBA32F;
        case rhi::Format::D32_FLOAT:         return GL_DEPTH_COMPONENT32F;
        case rhi::Format::D24_UNORM_S8_UINT: return GL_DEPTH24_STENCIL8;
        default:                             return GL_RGBA8;
    }
}

static GLenum ToGLBaseFormat(rhi::Format fmt)
{
    if (rhi::IsDepthFormat(fmt))
        return GL_DEPTH_COMPONENT;
    if (rhi::IsStencilFormat(fmt))
        return GL_DEPTH_STENCIL;
    switch (fmt)
    {
        case rhi::Format::BGRA8_UNORM:
        case rhi::Format::BGRA8_SRGB:        return GL_BGRA;
        default:                             return GL_RGBA;
    }
}

rhi::TextureHandle OpenGLDevice::createTexture(const rhi::TextureDesc& desc, const void* initialData)
{
    GLuint glTex = 0;
    glGenTextures(1, &glTex);
    if (!glTex)
        return {};

    GLenum target = GL_TEXTURE_2D;
    glBindTexture(target, glTex);
    glTexImage2D(target, 0, ToGLFormat(desc.Format),
                 desc.Extent.Width, desc.Extent.Height, 0,
                 ToGLBaseFormat(desc.Format), GL_UNSIGNED_BYTE, initialData);

    if (desc.MipLevels > 1)
        glGenerateMipmap(target);

    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(target, 0);

    auto handle = AllocHandle<rhi::TextureTag>();
    GpuSlot* slot = GetSlot(handle.GetId());
    if (slot)
        slot->GpuHandle = static_cast<uint64_t>(glTex);

    return handle;
}

// ============================================================
// Shader Creation (reuse existing OpenGLShader for SPIR-V)
// ============================================================

#include "Vulkitten/Renderer/Shader.h"

rhi::ShaderHandle OpenGLDevice::createShader(rhi::ShaderStage stage, const ShaderBytecode& bytecode)
{
    if (!bytecode.Data || bytecode.Size == 0)
        return {};

    GLenum glStage = GL_VERTEX_SHADER;
    if (rhi::HasStage(stage, rhi::ShaderStage::Fragment))
        glStage = GL_FRAGMENT_SHADER;
    else if (rhi::HasStage(stage, rhi::ShaderStage::Compute))
        glStage = GL_COMPUTE_SHADER;

    GLuint glShader = glCreateShader(glStage);
    if (!glShader)
        return {};

    const GLsizei count = static_cast<GLsizei>(bytecode.Size / sizeof(uint32_t));
    glShaderBinary(1, &glShader, GL_SHADER_BINARY_FORMAT_SPIR_V,
                   bytecode.Data, count * static_cast<GLsizei>(sizeof(uint32_t)));
    glSpecializeShader(glShader, bytecode.EntryPoint, 0, nullptr, nullptr);

    GLint compiled = 0;
    glGetShaderiv(glShader, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        GLint logLen = 0;
        glGetShaderiv(glShader, GL_INFO_LOG_LENGTH, &logLen);
        if (logLen > 1)
        {
            std::vector<char> log(static_cast<size_t>(logLen));
            glGetShaderInfoLog(glShader, logLen, nullptr, log.data());
            VKT_CORE_WARN("OpenGLDevice::createShader compile error: {0}", log.data());
        }
        glDeleteShader(glShader);
        return {};
    }

    auto handle = AllocHandle<rhi::ShaderTag>();
    GpuSlot* slot = GetSlot(handle.GetId());
    if (slot)
        slot->GpuHandle = static_cast<uint64_t>(glShader);

    return handle;
}

// ============================================================
// Pipeline Creation
// ============================================================

rhi::PipelineHandle OpenGLDevice::createPipeline(const rhi::PipelineDesc& desc)
{
    GLuint program = glCreateProgram();
    if (!program)
        return {};

    // Attach shaders if handles are valid
    if (desc.VertexShader.IsValid())
    {
        auto* vsSlot = GetSlot(desc.VertexShader.GetId());
        if (vsSlot && vsSlot->GpuHandle)
            glAttachShader(program, static_cast<GLuint>(vsSlot->GpuHandle));
    }
    if (desc.FragmentShader.IsValid())
    {
        auto* fsSlot = GetSlot(desc.FragmentShader.GetId());
        if (fsSlot && fsSlot->GpuHandle)
            glAttachShader(program, static_cast<GLuint>(fsSlot->GpuHandle));
    }
    glLinkProgram(program);

    auto handle = AllocHandle<rhi::PipelineTag>();
    GpuSlot* slot = GetSlot(handle.GetId());
    if (slot)
        slot->GpuHandle = static_cast<uint64_t>(program);

    // Store vertex layout for lazy VAO creation
    StorePipelineVertexLayout(handle.GetId(), desc.VertexLayout);

    return handle;
}

// ============================================================
// Geometry Creation
// ============================================================

rhi::GeometryHandle OpenGLDevice::createGeometry(const rhi::GeometryDesc& desc)
{
    auto handle = AllocHandle<rhi::GeometryTag>();
    StoreGeometryDesc(handle.GetId(), desc);
    return handle;
}

// ============================================================
// Sampler Creation
// ============================================================

rhi::SamplerHandle OpenGLDevice::createSampler(const rhi::SamplerDesc& desc)
{
    GLuint glSampler = 0;
    glGenSamplers(1, &glSampler);
    if (!glSampler)
        return {};

    GLenum minFilter = GL_LINEAR;
    if (desc.MinFilter == rhi::FilterMode::Nearest)
        minFilter = desc.Mip == rhi::MipMode::Linear ? GL_NEAREST_MIPMAP_LINEAR : GL_NEAREST;
    else if (desc.Mip == rhi::MipMode::Linear)
        minFilter = GL_LINEAR_MIPMAP_LINEAR;

    glSamplerParameteri(glSampler, GL_TEXTURE_MIN_FILTER, minFilter);
    glSamplerParameteri(glSampler, GL_TEXTURE_MAG_FILTER,
        desc.MagFilter == rhi::FilterMode::Linear ? GL_LINEAR : GL_NEAREST);

    auto ToGLWrap = [](rhi::WrapMode w) -> GLenum {
        switch (w) {
            case rhi::WrapMode::Repeat:         return GL_REPEAT;
            case rhi::WrapMode::ClampToEdge:    return GL_CLAMP_TO_EDGE;
            case rhi::WrapMode::ClampToBorder:  return GL_CLAMP_TO_BORDER;
            case rhi::WrapMode::MirroredRepeat: return GL_MIRRORED_REPEAT;
            default:                            return GL_REPEAT;
        }
    };
    glSamplerParameteri(glSampler, GL_TEXTURE_WRAP_S, ToGLWrap(desc.WrapU));
    glSamplerParameteri(glSampler, GL_TEXTURE_WRAP_T, ToGLWrap(desc.WrapV));
    glSamplerParameterf(glSampler, GL_TEXTURE_MAX_ANISOTROPY, desc.MaxAnisotropy);

    auto handle = AllocHandle<rhi::SamplerTag>();
    GpuSlot* slot = GetSlot(handle.GetId());
    if (slot)
        slot->GpuHandle = static_cast<uint64_t>(glSampler);

    return handle;
}

// ============================================================
// RenderPass / Framebuffer Creation
// ============================================================

rhi::RenderPassHandle OpenGLDevice::createRenderPass(const rhi::RenderPassDesc& /*desc*/)
{
    // [HACK: GL RenderPass = FBO metadata — 与 Framebuffer 一起管理]
    auto handle = AllocHandle<rhi::RenderPassTag>();
    return handle;
}

rhi::FramebufferHandle OpenGLDevice::createFramebuffer(const rhi::FramebufferDesc& desc)
{
    GLuint glFbo = 0;
    glGenFramebuffers(1, &glFbo);
    if (!glFbo)
        return {};

    glBindFramebuffer(GL_FRAMEBUFFER, glFbo);

    // Attach color textures
    for (size_t i = 0; i < desc.ColorAttachments.size(); i++)
    {
        GpuSlot* texSlot = GetSlot(desc.ColorAttachments[i].GetId());
        if (texSlot && texSlot->GpuHandle)
        {
            GLuint glTex = static_cast<GLuint>(texSlot->GpuHandle);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i),
                                   GL_TEXTURE_2D, glTex, 0);
        }
    }

    // Attach depth/stencil texture
    if (desc.DepthStencilAttachment.IsValid())
    {
        GpuSlot* dsSlot = GetSlot(desc.DepthStencilAttachment.GetId());
        if (dsSlot && dsSlot->GpuHandle)
        {
            GLuint glTex = static_cast<GLuint>(dsSlot->GpuHandle);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                   GL_TEXTURE_2D, glTex, 0);
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    auto handle = AllocHandle<rhi::FramebufferTag>();
    GpuSlot* slot = GetSlot(handle.GetId());
    if (slot)
        slot->GpuHandle = static_cast<uint64_t>(glFbo);

    return handle;
}

// ============================================================
// Window
// ============================================================

void OpenGLDevice::onResize(uint32_t width, uint32_t height)
{
    glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
}

// ============================================================
// Utilities
// ============================================================

void OpenGLDevice::waitIdle()
{
    glFinish();
}

// ============================================================
// Legacy
// ============================================================

void OpenGLDevice::Submit(FrameContext& /*frameContext*/)
{
    // Legacy — SwapBuffers now handled by endFrame()
}

// ---- Metadata accessors ----

void OpenGLDevice::StoreGeometryDesc(uint32_t id, const rhi::GeometryDesc& desc)
{
    m_GeometryDescs[id] = desc;
}

void OpenGLDevice::StorePipelineVertexLayout(uint32_t id, const std::vector<rhi::VertexAttribute>& layout)
{
    m_PipelineVertexLayouts[id] = layout;
}

const rhi::GeometryDesc* OpenGLDevice::GetGeometryDesc(uint32_t id) const
{
    auto it = m_GeometryDescs.find(id);
    return it != m_GeometryDescs.end() ? &it->second : nullptr;
}

const std::vector<rhi::VertexAttribute>* OpenGLDevice::GetPipelineVertexLayout(uint32_t id) const
{
    auto it = m_PipelineVertexLayouts.find(id);
    return it != m_PipelineVertexLayouts.end() ? &it->second : nullptr;
}

} // namespace Vulkitten

