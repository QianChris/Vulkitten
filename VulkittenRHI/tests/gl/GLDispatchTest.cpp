// ============================================================
// GL Dispatch Tests (requires GPU + OpenGL 4.6)
// Tests: DispatchCompute, DispatchIndirect
// Verifies results via buffer readback to CPU and data comparison.
// ============================================================

#include "GLTestFixture.hpp"
#include <glad/glad.h>
#include <cstring>
#include <cstdio>
#include <vector>

namespace rhi {
namespace test {

class GLDispatchTest : public GLTestFixture {
protected:
    void SetUp() override {
        GLTestFixture::SetUp();
        if (HasFatalFailure()) return;
    }
};

// ============================================================
// DispatchCompute tests
// ============================================================

TEST_F(GLDispatchTest, DispatchCompute_FillsBufferWithConstant) {
    if (HasFatalFailure()) return;
    auto& cmd = GetCommandBuffer();

    // Create output buffer (256 uints, initialized to 0)
    std::vector<uint32_t> initialData(256, 0);
    BufferHandle outputBuf = CreateStorageBuffer(
        initialData.size() * sizeof(uint32_t), initialData.data());
    ASSERT_TRUE(outputBuf.IsValid());

    ShaderHandle cs = LoadShaderFile(ShaderStage::Compute, "tests/gl/shaders/fill_constant.comp");
    ASSERT_TRUE(cs.IsValid()) << "Failed to load fill_constant.comp shader";

    PipelineHandle pso = CreateComputePipeline(cs);
    ASSERT_TRUE(pso.IsValid()) << "Failed to create compute pipeline";

    cmd.BindPipeline(pso);
    cmd.BindStorageBuffer(0, outputBuf, 0, 256 * sizeof(uint32_t));
    cmd.DispatchCompute(256, 1, 1);

    glFinish();
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    auto result = ReadBufferData(outputBuf, 0, 256 * sizeof(uint32_t));
    ASSERT_EQ(result.size(), 256u * sizeof(uint32_t));

    const uint32_t* data = reinterpret_cast<const uint32_t*>(result.data());
    for (size_t i = 0; i < 256; ++i) {
        EXPECT_EQ(data[i], 0xDEADBEEFu)
            << "Element " << i << " should be 0xDEADBEEF";
    }
}

TEST_F(GLDispatchTest, DispatchCompute_AddOneToEachElement) {
    if (HasFatalFailure()) return;
    auto& cmd = GetCommandBuffer();

    std::vector<uint32_t> initialData(256);
    for (uint32_t i = 0; i < 256; ++i) initialData[i] = i;

    BufferHandle buf = CreateStorageBuffer(
        initialData.size() * sizeof(uint32_t), initialData.data());
    ASSERT_TRUE(buf.IsValid());

    ShaderHandle cs = LoadShaderFile(ShaderStage::Compute, "tests/gl/shaders/add_one.comp");
    ASSERT_TRUE(cs.IsValid());
    PipelineHandle pso = CreateComputePipeline(cs);
    ASSERT_TRUE(pso.IsValid());

    cmd.BindPipeline(pso);
    cmd.BindStorageBuffer(0, buf, 0, 256 * sizeof(uint32_t));
    cmd.DispatchCompute(256, 1, 1);

    glFinish();
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    auto result = ReadBufferData(buf, 0, 256 * sizeof(uint32_t));
    ASSERT_EQ(result.size(), 256u * sizeof(uint32_t));

    const uint32_t* data = reinterpret_cast<const uint32_t*>(result.data());
    for (uint32_t i = 0; i < 256; ++i) {
        EXPECT_EQ(data[i], i + 1u)
            << "Element " << i << " should be " << (i + 1);
    }
}

TEST_F(GLDispatchTest, DispatchCompute_MultiGroup) {
    if (HasFatalFailure()) return;
    auto& cmd = GetCommandBuffer();

    std::vector<uint32_t> initialData(256, 0);
    BufferHandle buf = CreateStorageBuffer(
        initialData.size() * sizeof(uint32_t), initialData.data());
    ASSERT_TRUE(buf.IsValid());

    ShaderHandle cs = LoadShaderFile(ShaderStage::Compute, "tests/gl/shaders/fill_constant.comp");
    ASSERT_TRUE(cs.IsValid());
    PipelineHandle pso = CreateComputePipeline(cs);
    ASSERT_TRUE(pso.IsValid());

    cmd.BindPipeline(pso);
    cmd.BindStorageBuffer(0, buf, 0, 256 * sizeof(uint32_t));
    cmd.DispatchCompute(16, 16, 1); // 16*16 = 256 threads

    glFinish();
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    auto result = ReadBufferData(buf, 0, 256 * sizeof(uint32_t));
    const uint32_t* data = reinterpret_cast<const uint32_t*>(result.data());
    for (size_t i = 0; i < 256; ++i) {
        EXPECT_EQ(data[i], 0xDEADBEEFu)
            << "Multi-group: element " << i << " not written";
    }
}

TEST_F(GLDispatchTest, DispatchCompute_3DGroupCount) {
    if (HasFatalFailure()) return;
    auto& cmd = GetCommandBuffer();

    std::vector<uint32_t> initialData(256, 0);
    BufferHandle buf = CreateStorageBuffer(
        initialData.size() * sizeof(uint32_t), initialData.data());
    ASSERT_TRUE(buf.IsValid());

    ShaderHandle cs = LoadShaderFile(ShaderStage::Compute, "tests/gl/shaders/fill_constant.comp");
    ASSERT_TRUE(cs.IsValid());
    PipelineHandle pso = CreateComputePipeline(cs);
    ASSERT_TRUE(pso.IsValid());

    cmd.BindPipeline(pso);
    cmd.BindStorageBuffer(0, buf, 0, 256 * sizeof(uint32_t));
    cmd.DispatchCompute(8, 8, 4); // 8*8*4 = 256 threads in 3D

    glFinish();
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    auto result = ReadBufferData(buf, 0, 256 * sizeof(uint32_t));
    const uint32_t* data = reinterpret_cast<const uint32_t*>(result.data());
    for (size_t i = 0; i < 256; ++i) {
        EXPECT_EQ(data[i], 0xDEADBEEFu)
            << "3D dispatch: element " << i << " not written";
    }
}

// ============================================================
// DispatchIndirect tests
// ============================================================

TEST_F(GLDispatchTest, DispatchIndirect_UsesParametersFromBuffer) {
    if (HasFatalFailure()) return;
    auto& cmd = GetCommandBuffer();

    std::vector<uint32_t> initialData(256, 0);
    BufferHandle outputBuf = CreateStorageBuffer(
        initialData.size() * sizeof(uint32_t), initialData.data());
    ASSERT_TRUE(outputBuf.IsValid());

    BufferHandle indirectBuf = CreateIndirectDispatchBuffer(256, 1, 1);
    ASSERT_TRUE(indirectBuf.IsValid());

    ShaderHandle cs = LoadShaderFile(ShaderStage::Compute, "tests/gl/shaders/fill_constant.comp");
    ASSERT_TRUE(cs.IsValid());
    PipelineHandle pso = CreateComputePipeline(cs);
    ASSERT_TRUE(pso.IsValid());

    cmd.BindPipeline(pso);
    cmd.BindStorageBuffer(0, outputBuf, 0, 256 * sizeof(uint32_t));
    cmd.DispatchIndirect(indirectBuf, 0);

    glFinish();
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    auto result = ReadBufferData(outputBuf, 0, 256 * sizeof(uint32_t));
    const uint32_t* data = reinterpret_cast<const uint32_t*>(result.data());
    for (size_t i = 0; i < 256; ++i) {
        EXPECT_EQ(data[i], 0xDEADBEEFu)
            << "DispatchIndirect: element " << i << " not written";
    }
}

TEST_F(GLDispatchTest, DispatchIndirect_WithBufferOffset) {
    if (HasFatalFailure()) return;
    auto& cmd = GetCommandBuffer();

    std::vector<uint32_t> initialData(256, 0);
    BufferHandle outputBuf = CreateStorageBuffer(
        initialData.size() * sizeof(uint32_t), initialData.data());
    ASSERT_TRUE(outputBuf.IsValid());

    // Indirect buffer with padding before actual params
    std::vector<uint32_t> indirectData(16 + 3, 0); // 64 bytes padding + 12 bytes params
    indirectData[16] = 256; // groupX at offset 64
    indirectData[17] = 1;
    indirectData[18] = 1;

    BufferHandle indirectBuf = CreateStorageBuffer(
        indirectData.size() * sizeof(uint32_t), indirectData.data());
    ASSERT_TRUE(indirectBuf.IsValid());

    ShaderHandle cs = LoadShaderFile(ShaderStage::Compute, "tests/gl/shaders/fill_constant.comp");
    ASSERT_TRUE(cs.IsValid());
    PipelineHandle pso = CreateComputePipeline(cs);
    ASSERT_TRUE(pso.IsValid());

    cmd.BindPipeline(pso);
    cmd.BindStorageBuffer(0, outputBuf, 0, 256 * sizeof(uint32_t));
    cmd.DispatchIndirect(indirectBuf, 16 * sizeof(uint32_t));

    glFinish();
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    auto result = ReadBufferData(outputBuf, 0, 256 * sizeof(uint32_t));
    const uint32_t* data = reinterpret_cast<const uint32_t*>(result.data());
    for (size_t i = 0; i < 256; ++i) {
        EXPECT_EQ(data[i], 0xDEADBEEFu)
            << "DispatchIndirect+offset: element " << i << " not written";
    }
}

TEST_F(GLDispatchTest, DispatchCompute_CopyBetweenBuffers) {
    if (HasFatalFailure()) return;
    auto& cmd = GetCommandBuffer();

    std::vector<uint32_t> inputData(256);
    for (uint32_t i = 0; i < 256; ++i) inputData[i] = i * 3 + 7;

    BufferHandle inputBuf = CreateStorageBuffer(
        inputData.size() * sizeof(uint32_t), inputData.data());
    ASSERT_TRUE(inputBuf.IsValid());

    std::vector<uint32_t> outputInit(256, 0);
    BufferHandle outputBuf = CreateStorageBuffer(
        outputInit.size() * sizeof(uint32_t), outputInit.data());
    ASSERT_TRUE(outputBuf.IsValid());

    ShaderHandle cs = LoadShaderFile(ShaderStage::Compute, "tests/gl/shaders/copy_buffer.comp");
    ASSERT_TRUE(cs.IsValid());
    PipelineHandle pso = CreateComputePipeline(cs);
    ASSERT_TRUE(pso.IsValid());

    cmd.BindPipeline(pso);
    cmd.BindStorageBuffer(0, inputBuf, 0, 256 * sizeof(uint32_t));
    cmd.BindStorageBuffer(1, outputBuf, 0, 256 * sizeof(uint32_t));
    cmd.DispatchCompute(4, 1, 1); // 4 groups of 64 = 256

    glFinish();
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    auto result = ReadBufferData(outputBuf, 0, 256 * sizeof(uint32_t));
    const uint32_t* data = reinterpret_cast<const uint32_t*>(result.data());
    for (uint32_t i = 0; i < 256; ++i) {
        EXPECT_EQ(data[i], i * 3 + 7)
            << "Copy compute: element " << i << " mismatch";
    }
}

} // namespace test
} // namespace rhi
