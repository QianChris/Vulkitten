// ============================================================
// VK Command Recording Tests
//
// Tests that Draw/DrawIndirect/Dispatch/DispatchIndirect are
// correctly recorded into the VkCommandBuffer.
//
// NOTE: Full graphics draw tests require a render pass +
// framebuffer + pipeline setup, which is complex for headless
// testing. GL tests cover the draw semantics.
// ============================================================

#include "VKTestFixture.hpp"
#include <cstring>
#include <cstdio>
#include <vector>

namespace rhi {
namespace test {

class VKDrawTest : public VKTestFixture {
protected:
    void SetUp() override {
        VKTestFixture::SetUp();
        if (HasFatalFailure()) return;
    }
};

// Dispatch and DispatchIndirect can be recorded without render pass.
// We verify they execute correctly via buffer readback.

TEST_F(VKDrawTest, Dispatch_RecordAndSubmit_Executes) {
    if (HasFatalFailure()) return;
    auto& cmd = GetCommandBuffer();

    std::vector<uint32_t> init(256, 0);
    BufferHandle buf = CreateStorageBuffer(init.size() * sizeof(uint32_t), init.data());
    ASSERT_TRUE(buf.IsValid());

    // Load SPIR-V shader (must be compiled with glslangValidator first)
    ShaderHandle cs = LoadSpirvFile(ShaderStage::Compute,
        "tests/vk/shaders/fill_constant.comp.spv");
    if (!cs.IsValid()) {
        GTEST_SKIP() << "SPIR-V not found. Run: glslangValidator -V "
            "tests/gl/shaders/fill_constant.comp -o tests/vk/shaders/fill_constant.comp.spv";
        return;
    }

    PipelineHandle pso = CreateComputePipeline(cs);
    ASSERT_TRUE(pso.IsValid());

    cmd.BindPipeline(pso);
    cmd.BindStorageBuffer(0, buf, 0, 256 * sizeof(uint32_t));
    cmd.DispatchCompute(256, 1, 1);

    SubmitAndWait();

    auto result = ReadBufferData(buf, 0, 256 * sizeof(uint32_t));
    ASSERT_EQ(result.size(), 256u * sizeof(uint32_t));
    const uint32_t* data = reinterpret_cast<const uint32_t*>(result.data());
    for (size_t i = 0; i < 256; ++i) {
        EXPECT_EQ(data[i], 0xDEADBEEFu) << "Element " << i;
    }
}

TEST_F(VKDrawTest, DispatchIndirect_RecordAndSubmit_Executes) {
    if (HasFatalFailure()) return;
    auto& cmd = GetCommandBuffer();

    std::vector<uint32_t> init(256, 0);
    BufferHandle buf = CreateStorageBuffer(init.size() * sizeof(uint32_t), init.data());
    ASSERT_TRUE(buf.IsValid());

    BufferHandle indirectBuf = CreateIndirectDispatchBuffer(256, 1, 1);
    ASSERT_TRUE(indirectBuf.IsValid());

    ShaderHandle cs = LoadSpirvFile(ShaderStage::Compute,
        "tests/vk/shaders/fill_constant.comp.spv");
    if (!cs.IsValid()) { GTEST_SKIP() << "SPIR-V not found."; return; }

    PipelineHandle pso = CreateComputePipeline(cs);
    ASSERT_TRUE(pso.IsValid());

    cmd.BindPipeline(pso);
    cmd.BindStorageBuffer(0, buf, 0, 256 * sizeof(uint32_t));
    cmd.DispatchIndirect(indirectBuf, 0);

    SubmitAndWait();

    auto result = ReadBufferData(buf, 0, 256 * sizeof(uint32_t));
    ASSERT_EQ(result.size(), 256u * sizeof(uint32_t));
    const uint32_t* data = reinterpret_cast<const uint32_t*>(result.data());
    for (size_t i = 0; i < 256; ++i) {
        EXPECT_EQ(data[i], 0xDEADBEEFu) << "Element " << i;
    }
}

// Draw commands require an active render pass. We test that the
// RHI correctly records them without crashing (validation will
// complain, but the C++ layer should be fine).

TEST_F(VKDrawTest, Draw_RecordingDoesNotCrash) {
    if (HasFatalFailure()) return;
    auto& cmd = GetCommandBuffer();

    // These calls record Vulkan commands. Without a render pass,
    // validation layers will report errors, but the RHI must not
    // crash or throw C++ exceptions.
    EXPECT_NO_THROW(cmd.Draw(3));
    EXPECT_NO_THROW(cmd.Draw(6, 3));
    EXPECT_NO_THROW(cmd.Draw(3, 0, 2));
    SUCCEED();
}

TEST_F(VKDrawTest, DrawIndirect_RecordingDoesNotCrash) {
    if (HasFatalFailure()) return;
    auto& cmd = GetCommandBuffer();

    std::vector<uint32_t> drawCmd = {3, 1, 0, 0};
    BufferHandle indirectBuf = CreateStorageBuffer(
        drawCmd.size() * sizeof(uint32_t), drawCmd.data());
    ASSERT_TRUE(indirectBuf.IsValid());

    EXPECT_NO_THROW(cmd.DrawIndirect(indirectBuf, 0, 1, 16));
    SUCCEED();
}

} // namespace test
} // namespace rhi
