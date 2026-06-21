#include "vktpch.h"
#include "OpenGLDevice.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "Vulkitten/Renderer/FrameContext.h"
#include "Vulkitten/Perf/Instrumentor.h"

namespace Vulkitten {

OpenGLDevice::OpenGLDevice(void* nativeWindow)
    : m_NativeWindow(nativeWindow)
{
}

void OpenGLDevice::Init()
{
    VKT_PROFILE_RENDER_FUNCTION();
}

void OpenGLDevice::Shutdown()
{
    VKT_PROFILE_RENDER_FUNCTION();
}

// ---- Frame Lifecycle (stubs until Task 10) ----

FrameContext OpenGLDevice::beginFrame()
{
    // [HACK: 抽象层缺完整 IDevice beginFrame — Task 10 实现]
    return FrameContext{};
}

void OpenGLDevice::endFrame(FrameContext /*ctx*/)
{
    // [HACK: 抽象层缺完整 IDevice endFrame — Task 10 实现]
}

// ---- Command Buffer (stub until Task 11) ----

ICommandBuffer* OpenGLDevice::createCommandBuffer(FrameContext /*ctx*/)
{
    // [HACK: 抽象层缺 GLCommandBuffer — Task 11 创建]
    return nullptr;
}

// ---- Resource Creation (stubs until Task 10) ----

rhi::BufferHandle OpenGLDevice::createBuffer(const rhi::BufferDesc& /*desc*/, const void* /*initialData*/)
{
    // [HACK: 抽象层缺 GL Buffer 创建 — Task 10 实现]
    return {};
}

rhi::TextureHandle OpenGLDevice::createTexture(const rhi::TextureDesc& /*desc*/, const void* /*initialData*/)
{
    // [HACK: 抽象层缺 GL Texture 创建 — Task 10 实现]
    return {};
}

rhi::ShaderHandle OpenGLDevice::createShader(rhi::ShaderStage /*stage*/, const ShaderBytecode& /*bytecode*/)
{
    // [HACK: 抽象层缺 GL Shader 创建 — Task 10 实现]
    return {};
}

rhi::PipelineHandle OpenGLDevice::createPipeline(const rhi::PipelineDesc& /*desc*/)
{
    // [HACK: 抽象层缺 GL Pipeline 创建 — Task 10 实现]
    return {};
}

rhi::GeometryHandle OpenGLDevice::createGeometry(const rhi::GeometryDesc& /*desc*/)
{
    // [HACK: 抽象层缺 GL Geometry 创建 — Task 10 实现]
    return {};
}

rhi::SamplerHandle OpenGLDevice::createSampler(const rhi::SamplerDesc& /*desc*/)
{
    // [HACK: 抽象层缺 GL Sampler 创建 — Task 10 实现]
    return {};
}

rhi::RenderPassHandle OpenGLDevice::createRenderPass(const rhi::RenderPassDesc& /*desc*/)
{
    // [HACK: 抽象层缺 GL RenderPass/FBO 创建 — Task 10 实现]
    return {};
}

rhi::FramebufferHandle OpenGLDevice::createFramebuffer(const rhi::FramebufferDesc& /*desc*/)
{
    // [HACK: 抽象层缺 GL Framebuffer 创建 — Task 10 实现]
    return {};
}

// ---- Window ----

void OpenGLDevice::onResize(uint32_t /*width*/, uint32_t /*height*/)
{
    // [HACK: 抽象层缺 GL Resize 处理 — Task 10 实现]
}

// ---- Utilities ----

void OpenGLDevice::waitIdle()
{
    // [HACK: 抽象层缺完整 waitIdle — Task 10 实现 glFinish]
}

// ---- Legacy ----

void OpenGLDevice::Submit(FrameContext& /*frameContext*/)
{
    if (m_NativeWindow)
    {
        glfwSwapBuffers(static_cast<GLFWwindow*>(m_NativeWindow));
    }
}

} // namespace Vulkitten
