// ============================================================
// GLTestFixture - headless OpenGL test infrastructure
// ============================================================

#include "GLTestFixture.hpp"
#include "gl/GLDevice.hpp"
#include "gl/GLCommandBuffer.hpp"
#include "gl/GLResources.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cstring>
#include <cstdio>
#include <algorithm>
#include <fstream>

namespace rhi {
namespace test {

// ============================================================
// TestSurface
// ============================================================

TestSurface::TestSurface(uint32_t width, uint32_t height)
    : m_Width(width), m_Height(height)
{
}

SurfaceDesc TestSurface::GetDesc() const
{
    return {m_Width, m_Height};
}

void* TestSurface::GetNativeHandle() const
{
    return static_cast<void*>(m_Window);
}

// ============================================================
// Debug message capture (static)
// ============================================================

std::vector<std::string> GLTestFixture::s_DebugMessages;
std::mutex GLTestFixture::s_DebugMutex;

void GLTestFixture::DebugCallback(GLenum /*source*/, GLenum type, GLuint /*id*/,
                                   GLenum /*severity*/, GLsizei /*length*/,
                                   const GLchar* message, const void* /*userParam*/)
{
    std::lock_guard<std::mutex> lock(s_DebugMutex);

    const char* typeStr = "OTHER";
    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR:               typeStr = "ERROR"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: typeStr = "DEPRECATED"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  typeStr = "UNDEFINED"; break;
        case GL_DEBUG_TYPE_PORTABILITY:         typeStr = "PORTABILITY"; break;
        case GL_DEBUG_TYPE_PERFORMANCE:         typeStr = "PERFORMANCE"; break;
        case GL_DEBUG_TYPE_MARKER:              typeStr = "MARKER"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          typeStr = "PUSH_GROUP"; break;
        case GL_DEBUG_TYPE_POP_GROUP:           typeStr = "POP_GROUP"; break;
        default: break;
    }

    char buf[2048];
    snprintf(buf, sizeof(buf), "[%s] %s", typeStr, message);
    s_DebugMessages.push_back(buf);
}

void GLTestFixture::BeginDebugCapture()
{
    ClearDebugMessages();
    m_DebugCapturing = true;
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(DebugCallback, this);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
}

void GLTestFixture::EndDebugCapture()
{
    glDebugMessageCallback(nullptr, nullptr);
    glDisable(GL_DEBUG_OUTPUT);
    m_DebugCapturing = false;
}

void GLTestFixture::ClearDebugMessages()
{
    std::lock_guard<std::mutex> lock(s_DebugMutex);
    s_DebugMessages.clear();
}

// ============================================================
// SetUp / TearDown
// ============================================================

void GLTestFixture::SetUp()
{
    // Initialize GLFW if not already done
    if (!glfwInit())
    {
        FAIL() << "glfwInit() failed";
        return;
    }

    // Create hidden headless window with GL 4.6 core context
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);  // Enable debug output

    m_Window = glfwCreateWindow(static_cast<int>(kTestWidth),
                                 static_cast<int>(kTestHeight),
                                 "VulkittenRHI-GL-Test", nullptr, nullptr);
    if (!m_Window)
    {
        FAIL() << "glfwCreateWindow() failed - no GPU or GL 4.6 not supported";
        return;
    }

    glfwMakeContextCurrent(m_Window);

    // Load GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        FAIL() << "gladLoadGLLoader() failed";
        return;
    }

    // Set up the test surface with the window
    m_Surface.SetWindow(m_Window);

    // Create GL device (owns GL context via surface)
    m_Device = std::make_unique<GLDevice>(&m_Surface, m_Resources);
    m_Device->Init();

    // Create an initial command buffer for the first frame
    m_CurrentFrame = m_Device->BeginFrame();
    m_CommandBuffer = m_Device->CreateCommandBuffer(m_CurrentFrame);
    m_CommandBuffer->Begin();

    // Configure viewport
    glViewport(0, 0, static_cast<GLsizei>(kTestWidth), static_cast<GLsizei>(kTestHeight));

    printf("[GLTest] GL Vendor:   %s\n", glGetString(GL_VENDOR));
    printf("[GLTest] GL Renderer: %s\n", glGetString(GL_RENDERER));
    printf("[GLTest] GL Version:  %s\n", glGetString(GL_VERSION));
}

void GLTestFixture::TearDown()
{
    if (m_CommandBuffer)
    {
        m_CommandBuffer->End();
        m_CommandBuffer.reset();
    }

    if (m_Device)
    {
        m_Device->EndFrame(m_CurrentFrame);
        // Shutdown is called by ~GLDevice() via m_Device.reset() below;
        // calling it explicitly here would double-trigger glFinish()
        m_Device.reset();
    }

    m_Resources.DestroyAll();

    if (m_Window)
    {
        glfwDestroyWindow(m_Window);
        m_Window = nullptr;
    }
}

// ============================================================
// Command Buffer Access
// ============================================================

ICommandBuffer& GLTestFixture::GetCommandBuffer()
{
    return *m_CommandBuffer;
}

// ============================================================
// Quick Resource Creation
// ============================================================

BufferHandle GLTestFixture::CreateVertexBuffer(const void* data, uint64_t size)
{
    BufferDesc desc;
    desc.Size = size;
    desc.Usage = BufferUsage::Vertex;
    desc.Memory = MemoryProperty::HostVisible;
    return m_Device->CreateBuffer(desc, data);
}

BufferHandle GLTestFixture::CreateIndexBuffer(const void* data, uint64_t size, IndexType type)
{
    BufferDesc desc;
    desc.Size = size;
    desc.Usage = BufferUsage::Index;
    desc.Memory = MemoryProperty::HostVisible;
    // Store as u32 or u16 - the type parameter is stored in GeometryDesc, not BufferDesc
    return m_Device->CreateBuffer(desc, data);
}

BufferHandle GLTestFixture::CreateUniformBuffer(uint64_t size, const void* data)
{
    BufferDesc desc;
    desc.Size = size;
    desc.Usage = BufferUsage::Uniform;
    desc.Memory = MemoryProperty::HostVisible;
    desc.CpuAccessible = true;
    return m_Device->CreateBuffer(desc, data);
}

BufferHandle GLTestFixture::CreateStorageBuffer(uint64_t size, const void* data)
{
    BufferDesc desc;
    desc.Size = size;
    desc.Usage = BufferUsage::Storage;
    desc.Memory = MemoryProperty::HostVisible;
    desc.CpuAccessible = true;
    return m_Device->CreateBuffer(desc, data);
}

BufferHandle GLTestFixture::CreateIndirectDrawBuffer(const std::vector<uint32_t>& drawParams)
{
    BufferDesc desc;
    desc.Size = drawParams.size() * sizeof(uint32_t);
    desc.Usage = BufferUsage::Storage | BufferUsage::Indirect;
    desc.Memory = MemoryProperty::HostVisible;
    desc.CpuAccessible = true;
    return m_Device->CreateBuffer(desc, drawParams.data());
}

BufferHandle GLTestFixture::CreateIndirectDispatchBuffer(uint32_t x, uint32_t y, uint32_t z)
{
    uint32_t params[3] = {x, y, z};
    BufferDesc desc;
    desc.Size = sizeof(params);
    desc.Usage = BufferUsage::Storage | BufferUsage::Indirect;
    desc.Memory = MemoryProperty::HostVisible;
    desc.CpuAccessible = true;
    return m_Device->CreateBuffer(desc, params);
}

ShaderHandle GLTestFixture::CreateShaderFromGLSL(ShaderStage stage, const char* source)
{
    ShaderBytecode bc;
    bc.Data = reinterpret_cast<const uint8_t*>(source);
    bc.Size = std::strlen(source);
    bc.EntryPoint = "main";
    return m_Device->CreateShader(stage, bc);
}

PipelineHandle GLTestFixture::CreateGraphicsPipeline(
    ShaderHandle vs, ShaderHandle fs,
    const std::vector<VertexAttribute>& layout,
    RenderPassHandle rp,
    RasterState::CullMode cull)
{
    PipelineDesc desc;
    desc.VertexShader = vs;
    desc.FragmentShader = fs;
    desc.VertexLayout = layout;
    desc.RenderPass = rp;
    desc.Raster.Cull = cull;

    return m_Device->CreatePipeline(desc);
}

PipelineHandle GLTestFixture::CreateComputePipeline(ShaderHandle cs)
{
    PipelineDesc desc;
    desc.ComputeShader = cs;
    return m_Device->CreatePipeline(desc);
}

GeometryHandle GLTestFixture::CreateSimpleGeometry(BufferHandle vb, uint32_t vtxCount,
                                                     uint32_t stride)
{
    GeometryDesc desc;
    desc.VertexBuffers[0] = vb;
    desc.VertexBufferCount = 1;
    desc.VertexCount = vtxCount;
    return m_Device->CreateGeometry(desc);
}

FramebufferHandle GLTestFixture::CreateOffscreenFramebuffer(
    RenderPassHandle rp, TextureHandle colorTex,
    uint32_t w, uint32_t h)
{
    FramebufferDesc desc;
    desc.RenderPass = rp;
    desc.ColorAttachments = { colorTex };
    desc.Width = w;
    desc.Height = h;
    return m_Device->CreateFramebuffer(desc);
}

TextureHandle GLTestFixture::CreateRenderTarget(uint32_t w, uint32_t h, Format fmt)
{
    TextureDesc desc;
    desc.Type = TextureType::Texture2D;
    desc.Format = fmt;
    desc.Extent = {w, h, 1};
    desc.Usage = TextureUsage::ColorAttachment | TextureUsage::Sampled;
    desc.Memory = MemoryProperty::DeviceLocal;
    return m_Device->CreateTexture(desc, nullptr);
}

RenderPassHandle GLTestFixture::CreateSimpleRenderPass(Format colorFmt)
{
    RenderPassDesc desc;
    AttachmentDesc colorAttach;
    colorAttach.Format = colorFmt;
    colorAttach.LoadOp = LoadOp::Clear;
    colorAttach.StoreOp = StoreOp::Store;
    colorAttach.InitialLayout = ImageLayout::Undefined;
    colorAttach.FinalLayout = ImageLayout::ColorAttachment;
    colorAttach.ClearValue.Color.RGBA = {0.0f, 0.0f, 0.0f, 0.0f};
    desc.ColorAttachments.push_back(colorAttach);

    SubpassDesc subpass;
    subpass.ColorAttachments.push_back(0);
    desc.Subpasses.push_back(subpass);

    return m_Device->CreateRenderPass(desc);
}

// ============================================================
// Pixel Readback
// ============================================================

std::vector<uint8_t> GLTestFixture::ReadPixels(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    std::vector<uint8_t> pixels(w * h * 4);
    glReadPixels(static_cast<GLint>(x), static_cast<GLint>(y),
                 static_cast<GLsizei>(w), static_cast<GLsizei>(h),
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    return pixels;
}

std::vector<uint8_t> GLTestFixture::ReadAllPixels()
{
    return ReadPixels(0, 0, kTestWidth, kTestHeight);
}

// ============================================================
// Buffer Readback
// ============================================================

std::vector<uint8_t> GLTestFixture::ReadBufferData(BufferHandle handle, uint64_t offset, uint64_t size)
{
    // Access the GL buffer through the device
    auto* buf = dynamic_cast<GLBufferResource*>(m_Device->GetBuffer(handle));
    if (!buf || !buf->GetGLBuffer())
        return {};

    // Bind to COPY_READ target and read back
    GLuint glBuf = buf->GetGLBuffer();
    glBindBuffer(GL_COPY_READ_BUFFER, glBuf);

    std::vector<uint8_t> data(size);
    glGetBufferSubData(GL_COPY_READ_BUFFER, static_cast<GLintptr>(offset),
                       static_cast<GLsizeiptr>(size), data.data());

    glBindBuffer(GL_COPY_READ_BUFFER, 0);
    return data;
}

// ============================================================
// File-based Shader Loading
// ============================================================

std::string GLTestFixture::ReadShaderFile(const char* filepath)
{
    std::ifstream f(filepath, std::ios::binary | std::ios::ate);
    if (!f) return {};
    size_t size = static_cast<size_t>(f.tellg());
    f.seekg(0);
    std::string content(size, '\0');
    f.read(&content[0], static_cast<std::streamsize>(size));
    return content;
}

ShaderHandle GLTestFixture::LoadShaderFile(ShaderStage stage, const char* filepath)
{
    // Prepend VULKITTEN_RHI_DIR (set by CMake) to resolve relative paths
    std::string fullPath = std::string(VULKITTEN_RHI_DIR) + "/" + filepath;
    std::string source = ReadShaderFile(fullPath.c_str());
    if (source.empty())
    {
        // Fallback: try path as-is (e.g. when running from VulkittenRHI dir)
        source = ReadShaderFile(filepath);
    }
    if (source.empty())
    {
        fprintf(stderr, "LoadShaderFile: failed to read %s (also tried %s)\n",
                filepath, fullPath.c_str());
        return {};
    }
    return CreateShaderFromGLSL(stage, source.c_str());
}

} // namespace test
} // namespace rhi
