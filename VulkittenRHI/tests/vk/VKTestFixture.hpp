#pragma once

#include "rhi/IRenderDevice.hpp"
#include "rhi/ICommandBuffer.hpp"
#include "rhi/ResourceManager.hpp"
#include "rhi/ResourceDescs.hpp"
#include "rhi/ISurface.hpp"
#include "rhi/Core/Handle.hpp"
#include "rhi/Core/Types.hpp"
#include "rhi/Core/Format.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <fstream>

struct GLFWwindow;

namespace rhi {
namespace test {

class TestSurface : public ISurface {
public:
    TestSurface(uint32_t w, uint32_t h) : m_Width(w), m_Height(h) {}
    SurfaceDesc GetDesc() const override { return {m_Width, m_Height}; }
    void* GetNativeHandle() const override { return m_Window; }
    void SetWindow(GLFWwindow* w) { m_Window = w; }
private:
    uint32_t m_Width, m_Height;
    GLFWwindow* m_Window = nullptr;
};

class VKTestFixture : public ::testing::Test {
protected:
    void SetUp() override;
    void TearDown() override;

    IRenderDevice&   GetDevice()      { return *m_Device; }
    ResourceManager& GetResources()   { return m_Resources; }
    ICommandBuffer&  GetCommandBuffer();

    // Quick resource creation
    BufferHandle   CreateVertexBuffer(const void* data, uint64_t size);
    BufferHandle   CreateStorageBuffer(uint64_t size, const void* data = nullptr);
    BufferHandle   CreateIndirectDispatchBuffer(uint32_t x, uint32_t y, uint32_t z);

    ShaderHandle   LoadSpirvFile(ShaderStage stage, const char* filepath);
    PipelineHandle CreateComputePipeline(ShaderHandle cs);

    // Submit the recorded command buffer to the GPU and wait for completion.
    // Required because Vulkan executes commands only on explicit submission,
    // unlike OpenGL's immediate execution.
    void SubmitAndWait();

    GeometryHandle CreateSimpleGeometry(BufferHandle vb, uint32_t vtxCount, uint32_t stride);

    // Buffer readback
    std::vector<uint8_t> ReadBufferData(BufferHandle handle, uint64_t offset, uint64_t size);

    static std::vector<uint8_t> ReadFile(const char* path);

    static constexpr uint32_t kTestWidth = 64;
    static constexpr uint32_t kTestHeight = 64;

private:
    GLFWwindow* m_Window = nullptr;
    TestSurface m_Surface{kTestWidth, kTestHeight};
    ResourceManager m_Resources;
    std::unique_ptr<IRenderDevice> m_Device;
    std::unique_ptr<ICommandBuffer> m_CommandBuffer;
    FrameContext m_CurrentFrame;
};

} // namespace test
} // namespace rhi
