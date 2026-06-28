#pragma once

#include "rhi/IRenderDevice.hpp"
#include "rhi/ICommandBuffer.hpp"
#include "rhi/ResourceManager.hpp"
#include "rhi/ResourceDescs.hpp"
#include "rhi/ISurface.hpp"
#include "rhi/Core/Handle.hpp"
#include "rhi/Core/Types.hpp"
#include "rhi/Core/Format.hpp"

#include <glad/glad.h>     // for GLenum in debug callback signature
#include <gtest/gtest.h>

#include <memory>
#include <vector>
#include <string>
#include <array>
#include <mutex>
#include <cstdint>

struct GLFWwindow;

namespace rhi {
namespace test {

// ============================================================
// TestSurface - minimal ISurface for headless offscreen testing
// ============================================================
class TestSurface : public ISurface
{
public:
    TestSurface(uint32_t width, uint32_t height);
    SurfaceDesc GetDesc() const override;
    void* GetNativeHandle() const override;
    void SetWindow(GLFWwindow* w) { m_Window = w; }

private:
    uint32_t m_Width;
    uint32_t m_Height;
    GLFWwindow* m_Window = nullptr;
};

// ============================================================
// GLTestFixture - headless OpenGL context for GPU tests
//
// Creates a hidden GLFW window + GL 4.6 core context.
// Provides helpers for shader creation, pixel readback,
// buffer readback, and debug message capture.
//
// Usage:
//   TEST_F(GLTestFixture, MyTest) {
//       auto& device = GetDevice();
//       auto& cmd = GetCommandBuffer();
//       // ... create resources, record commands ...
//       ReadPixels(...);
//   }
// ============================================================

class GLTestFixture : public ::testing::Test
{
protected:
    // ---- GTest lifecycle ----
    void SetUp() override;
    void TearDown() override;

    // ---- Device access ----
    IRenderDevice&    GetDevice()       { return *m_Device; }
    ResourceManager&  GetResources()    { return m_Resources; }
    ICommandBuffer&   GetCommandBuffer();

    // ---- Quick resource creation ----
    BufferHandle   CreateVertexBuffer(const void* data, uint64_t size);
    BufferHandle   CreateIndexBuffer(const void* data, uint64_t size, IndexType type = IndexType::UInt32);
    BufferHandle   CreateUniformBuffer(uint64_t size, const void* data = nullptr);
    BufferHandle   CreateStorageBuffer(uint64_t size, const void* data = nullptr);
    BufferHandle   CreateIndirectDrawBuffer(const std::vector<uint32_t>& drawParams);
    BufferHandle   CreateIndirectDispatchBuffer(uint32_t x, uint32_t y, uint32_t z);

    ShaderHandle   CreateShaderFromGLSL(ShaderStage stage, const char* source);
    ShaderHandle   LoadShaderFile(ShaderStage stage, const char* filepath);
    static std::string ReadShaderFile(const char* filepath);
    PipelineHandle CreateGraphicsPipeline(ShaderHandle vs, ShaderHandle fs,
                                          const std::vector<VertexAttribute>& layout,
                                          RenderPassHandle rp,
                                          RasterState::CullMode cull = RasterState::CullMode::None);
    PipelineHandle CreateComputePipeline(ShaderHandle cs);

    GeometryHandle CreateSimpleGeometry(BufferHandle vb, uint32_t vtxCount,
                                        uint32_t stride = 6 * sizeof(float));

    FramebufferHandle CreateOffscreenFramebuffer(RenderPassHandle rp,
                                                  TextureHandle colorTex,
                                                  uint32_t w, uint32_t h);
    TextureHandle  CreateRenderTarget(uint32_t w, uint32_t h, Format fmt = Format::RGBA8_UNORM);

    // ---- Render pass helpers ----
    RenderPassHandle CreateSimpleRenderPass(Format colorFmt = Format::RGBA8_UNORM);

    // ---- Pixel readback ----
    // Read back RGBA8 pixels from the currently bound framebuffer
    std::vector<uint8_t> ReadPixels(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
    // Read back entire framebuffer
    std::vector<uint8_t> ReadAllPixels();

    // ---- Buffer readback ----
    std::vector<uint8_t> ReadBufferData(BufferHandle handle, uint64_t offset, uint64_t size);

    // ---- Debug message capture ----
    void BeginDebugCapture();
    void EndDebugCapture();
    const std::vector<std::string>& GetCapturedDebugMessages() const { return s_DebugMessages; }
    void ClearDebugMessages();

    // ---- Test window dimensions ----
    static constexpr uint32_t kTestWidth = 64;
    static constexpr uint32_t kTestHeight = 64;

private:
    GLFWwindow* m_Window = nullptr;
    TestSurface m_Surface{kTestWidth, kTestHeight};
    ResourceManager m_Resources;

    std::unique_ptr<IRenderDevice> m_Device;
    std::unique_ptr<ICommandBuffer> m_CommandBuffer;
    FrameContext m_CurrentFrame;

    // Debug message capture
    static std::vector<std::string> s_DebugMessages;
    static std::mutex s_DebugMutex;
    bool m_DebugCapturing = false;

    static void DebugCallback(GLenum source, GLenum type, GLuint id,
                              GLenum severity, GLsizei length,
                              const GLchar* message, const void* userParam);
};

} // namespace test
} // namespace rhi
