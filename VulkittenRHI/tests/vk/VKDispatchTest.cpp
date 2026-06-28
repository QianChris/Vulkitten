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

    SubmitAndWait();

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
    SubmitAndWait();

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
    // Use 1D dispatch: fill_constant uses gl_GlobalInvocationID.x directly
    cmd.DispatchCompute(256, 1, 1);
    SubmitAndWait();

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
    SubmitAndWait();

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

    // Test multi-slot buffer binding. The copy_buffer shader reads from
    // slot 0 and writes to slot 1. We dispatch the same work twice:
    // first fill bufA (slot 0), then read bufA→write bufB (slot 1).

    std::vector<uint32_t> dataA(256, 42); // input data
    std::vector<uint32_t> dataB(256, 0);  // output buffer (zeroed)

    BufferHandle bufA = CreateStorageBuffer(256 * sizeof(uint32_t), dataA.data());
    BufferHandle bufB = CreateStorageBuffer(256 * sizeof(uint32_t), dataB.data());
    ASSERT_TRUE(bufA.IsValid());
    ASSERT_TRUE(bufB.IsValid());

    ShaderHandle cs = LoadSpirvFile(ShaderStage::Compute,
        "tests/vk/shaders/copy_buffer.comp.spv");
    if (!cs.IsValid()) { GTEST_SKIP() << "SPIR-V copy_buffer.comp.spv not found"; return; }
    PipelineHandle pso = CreateComputePipeline(cs);
    ASSERT_TRUE(pso.IsValid());

    cmd.BindPipeline(pso);
    cmd.BindStorageBuffer(0, bufA, 0, 256 * sizeof(uint32_t));
    cmd.BindStorageBuffer(1, bufB, 0, 256 * sizeof(uint32_t));
    // 1D dispatch: 256 threads, copy_buffer has local_size_x=1 now.
    // Actually copy_buffer uses local_size_x=64. Use 4 groups.
    cmd.DispatchCompute(4, 1, 1);
    SubmitAndWait();

    // bufB should now contain bufA's data (42)
    auto resultB = ReadBufferData(bufB, 0, 256 * sizeof(uint32_t));
    ASSERT_EQ(resultB.size(), 256u * sizeof(uint32_t));
    const uint32_t* dB = reinterpret_cast<const uint32_t*>(resultB.data());
    for (uint32_t i = 0; i < 256; ++i)
        EXPECT_EQ(dB[i], 42u) << "Slot 1 output: elem " << i << " mismatch";
}

} // namespace test
} // namespace rhi
