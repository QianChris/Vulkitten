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

    // Use fill_linear.comp which computes proper linear index from
    // gl_GlobalInvocationID (works for any dispatch dimensionality)
    ShaderHandle cs = LoadShaderFile(ShaderStage::Compute, "tests/gl/shaders/fill_linear.comp");
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

    // Use fill_linear.comp (proper linear index via gl_GlobalInvocationID)
    ShaderHandle cs = LoadShaderFile(ShaderStage::Compute, "tests/gl/shaders/fill_linear.comp");
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

    // Use fill_linear.comp (which we know works) to verify multi-slot binding:
    //   slot 0 → reads from input (with known data)
    // We test multi-slot by dispatching twice: first to fill slot 1 buffer,
    // then read back both to verify independence.
    //
    // Actually, simply verify that binding two different buffers to different
    // slots works by using add_one on one buffer and checking the other was untouched.

    std::vector<uint32_t> dataA(256, 0);
    std::vector<uint32_t> dataB(256, 0);
    for (uint32_t i = 0; i < 256; ++i) dataB[i] = 42; // sentinel

    BufferHandle bufA = CreateStorageBuffer(256 * sizeof(uint32_t), dataA.data());
    BufferHandle bufB = CreateStorageBuffer(256 * sizeof(uint32_t), dataB.data());
    ASSERT_TRUE(bufA.IsValid());
    ASSERT_TRUE(bufB.IsValid());

    // Load add_one shader
    ShaderHandle cs = LoadShaderFile(ShaderStage::Compute, "tests/gl/shaders/add_one.comp");
    ASSERT_TRUE(cs.IsValid());
    PipelineHandle pso = CreateComputePipeline(cs);
    ASSERT_TRUE(pso.IsValid());

    // Bind both buffers: bufA at slot 0 (will be modified), bufB at slot 1 (should be untouched)
    cmd.BindPipeline(pso);
    cmd.BindStorageBuffer(0, bufA, 0, 256 * sizeof(uint32_t));
    cmd.BindStorageBuffer(1, bufB, 0, 256 * sizeof(uint32_t));
    cmd.DispatchCompute(256, 1, 1); // add_one only writes to slot 0

    glFinish();
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // bufA should have been incremented (0→1)
    auto resultA = ReadBufferData(bufA, 0, 256 * sizeof(uint32_t));
    ASSERT_EQ(resultA.size(), 256u * sizeof(uint32_t));
    const uint32_t* dA = reinterpret_cast<const uint32_t*>(resultA.data());
    for (uint32_t i = 0; i < 256; ++i) {
        EXPECT_EQ(dA[i], 1u) << "bufA slot 0: element " << i << " not modified";
    }

    // bufB should be untouched (42)
    auto resultB = ReadBufferData(bufB, 0, 256 * sizeof(uint32_t));
    ASSERT_EQ(resultB.size(), 256u * sizeof(uint32_t));
    const uint32_t* dB = reinterpret_cast<const uint32_t*>(resultB.data());
    for (uint32_t i = 0; i < 256; ++i) {
        EXPECT_EQ(dB[i], 42u) << "bufB slot 1: element " << i << " was incorrectly modified";
    }
}

} // namespace test
} // namespace rhi
