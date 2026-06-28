// VK Dispatch Tests (requires Vulkan GPU)
// Tests: DispatchCompute, DispatchIndirect
// Verifies results via buffer readback to CPU.
// ============================================================

#include "VKTestFixture.hpp"
#include <cstring>
#include <cstdio>
#include <vector>

namespace rhi {
namespace test {

class VKDispatchTest : public VKTestFixture {
protected:
    void SetUp() override {
        VKTestFixture::SetUp();
        if (HasFatalFailure()) return;
    }
};

TEST_F(VKDispatchTest, DispatchCompute_FillsBufferWithConstant) {
    if (HasFatalFailure()) return;
    auto& cmd = GetCommandBuffer();

    std::vector<uint32_t> initialData(256, 0);
    BufferHandle outputBuf = CreateStorageBuffer(
        initialData.size() * sizeof(uint32_t), initialData.data());
    ASSERT_TRUE(outputBuf.IsValid());

    ShaderHandle cs = LoadSpirvFile(ShaderStage::Compute,
        "tests/vk/shaders/fill_constant.comp.spv");
    if (!cs.IsValid()) {
        GTEST_SKIP() << "SPIR-V compute shader not found. "
            "Run: glslangValidator -V tests/gl/shaders/fill_constant.comp "
            "-o tests/vk/shaders/fill_constant.comp.spv";
        return;
    }
    PipelineHandle pso = CreateComputePipeline(cs);
    ASSERT_TRUE(pso.IsValid());

    cmd.BindPipeline(pso);
    cmd.BindStorageBuffer(0, outputBuf, 0, 256 * sizeof(uint32_t));
    cmd.DispatchCompute(256, 1, 1);

    GetDevice().WaitIdle();

    auto result = ReadBufferData(outputBuf, 0, 256 * sizeof(uint32_t));
    ASSERT_EQ(result.size(), 256u * sizeof(uint32_t));
    const uint32_t* data = reinterpret_cast<const uint32_t*>(result.data());
    for (size_t i = 0; i < 256; ++i) {
        EXPECT_EQ(data[i], 0xDEADBEEFu)
            << "Element " << i << " should be 0xDEADBEEF";
    }
}

TEST_F(VKDispatchTest, DispatchCompute_AddOneToEachElement) {
    if (HasFatalFailure()) return;
    auto& cmd = GetCommandBuffer();

    std::vector<uint32_t> initialData(256);
    for (uint32_t i = 0; i < 256; ++i) initialData[i] = i;
    BufferHandle buf = CreateStorageBuffer(
        initialData.size() * sizeof(uint32_t), initialData.data());
    ASSERT_TRUE(buf.IsValid());

    ShaderHandle cs = LoadSpirvFile(ShaderStage::Compute,
        "tests/vk/shaders/add_one.comp.spv");
    if (!cs.IsValid()) { GTEST_SKIP() << "SPIR-V add_one.comp.spv not found"; return; }
    PipelineHandle pso = CreateComputePipeline(cs);
    ASSERT_TRUE(pso.IsValid());

    cmd.BindPipeline(pso);
    cmd.BindStorageBuffer(0, buf, 0, 256 * sizeof(uint32_t));
    cmd.DispatchCompute(256, 1, 1);
    GetDevice().WaitIdle();

    auto result = ReadBufferData(buf, 0, 256 * sizeof(uint32_t));
    ASSERT_EQ(result.size(), 256u * sizeof(uint32_t));
    const uint32_t* data = reinterpret_cast<const uint32_t*>(result.data());
    for (uint32_t i = 0; i < 256; ++i) {
        EXPECT_EQ(data[i], i + 1u) << "Element " << i << " mismatch";
    }
}

TEST_F(VKDispatchTest, DispatchCompute_MultiGroup) {
    if (HasFatalFailure()) return;
    auto& cmd = GetCommandBuffer();

    std::vector<uint32_t> initialData(256, 0);
    BufferHandle buf = CreateStorageBuffer(
        initialData.size() * sizeof(uint32_t), initialData.data());
    ASSERT_TRUE(buf.IsValid());

    ShaderHandle cs = LoadSpirvFile(ShaderStage::Compute,
        "tests/vk/shaders/fill_constant.comp.spv");
    if (!cs.IsValid()) { GTEST_SKIP() << "SPIR-V fill_constant.comp.spv not found"; return; }
    PipelineHandle pso = CreateComputePipeline(cs);
    ASSERT_TRUE(pso.IsValid());

    cmd.BindPipeline(pso);
    cmd.BindStorageBuffer(0, buf, 0, 256 * sizeof(uint32_t));
    cmd.DispatchCompute(16, 16, 1); // 16*16=256 threads
    GetDevice().WaitIdle();

    auto result = ReadBufferData(buf, 0, 256 * sizeof(uint32_t));
    const uint32_t* data = reinterpret_cast<const uint32_t*>(result.data());
    for (size_t i = 0; i < 256; ++i) {
        EXPECT_EQ(data[i], 0xDEADBEEFu) << "Multi-group: elem " << i << " not written";
    }
}

TEST_F(VKDispatchTest, DispatchIndirect_UsesParametersFromBuffer) {
    if (HasFatalFailure()) return;
    auto& cmd = GetCommandBuffer();

    std::vector<uint32_t> initialData(256, 0);
    BufferHandle outputBuf = CreateStorageBuffer(
        initialData.size() * sizeof(uint32_t), initialData.data());
    ASSERT_TRUE(outputBuf.IsValid());

    BufferHandle indirectBuf = CreateIndirectDispatchBuffer(256, 1, 1);
    ASSERT_TRUE(indirectBuf.IsValid());

    ShaderHandle cs = LoadSpirvFile(ShaderStage::Compute,
        "tests/vk/shaders/fill_constant.comp.spv");
    if (!cs.IsValid()) { GTEST_SKIP() << "SPIR-V fill_constant.comp.spv not found"; return; }
    PipelineHandle pso = CreateComputePipeline(cs);
    ASSERT_TRUE(pso.IsValid());

    cmd.BindPipeline(pso);
    cmd.BindStorageBuffer(0, outputBuf, 0, 256 * sizeof(uint32_t));
    cmd.DispatchIndirect(indirectBuf, 0);
    GetDevice().WaitIdle();

    auto result = ReadBufferData(outputBuf, 0, 256 * sizeof(uint32_t));
    const uint32_t* data = reinterpret_cast<const uint32_t*>(result.data());
    for (size_t i = 0; i < 256; ++i) {
        EXPECT_EQ(data[i], 0xDEADBEEFu)
            << "DispatchIndirect: element " << i << " not written";
    }
}

TEST_F(VKDispatchTest, DispatchCompute_CopyBetweenBuffers) {
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

    ShaderHandle cs = LoadSpirvFile(ShaderStage::Compute,
        "tests/vk/shaders/copy_buffer.comp.spv");
    if (!cs.IsValid()) { GTEST_SKIP() << "SPIR-V copy_buffer.comp.spv not found"; return; }
    PipelineHandle pso = CreateComputePipeline(cs);
    ASSERT_TRUE(pso.IsValid());

    cmd.BindPipeline(pso);
    cmd.BindStorageBuffer(0, inputBuf, 0, 256 * sizeof(uint32_t));
    cmd.BindStorageBuffer(1, outputBuf, 0, 256 * sizeof(uint32_t));
    cmd.DispatchCompute(4, 1, 1);
    GetDevice().WaitIdle();

    auto result = ReadBufferData(outputBuf, 0, 256 * sizeof(uint32_t));
    const uint32_t* data = reinterpret_cast<const uint32_t*>(result.data());
    for (uint32_t i = 0; i < 256; ++i) {
        EXPECT_EQ(data[i], i * 3 + 7) << "Element " << i << " mismatch";
    }
}

} // namespace test
} // namespace rhi
