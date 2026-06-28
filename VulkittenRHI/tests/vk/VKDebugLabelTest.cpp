// VK Debug Label Tests (requires Vulkan + VK_EXT_debug_utils)
// Tests: BeginDebugLabel, EndDebugLabel, InsertDebugMarker
// ============================================================

#include "VKTestFixture.hpp"
#include <cstring>
#include <cstdio>

namespace rhi {
namespace test {

class VKDebugLabelTest : public VKTestFixture {
protected:
    void SetUp() override {
        VKTestFixture::SetUp();
        if (HasFatalFailure()) return;
    }
};

// These tests verify the debug label API doesn't crash.
// Actual debug message capture would require VK_EXT_debug_utils
// callback setup which is complex.

TEST_F(VKDebugLabelTest, BeginEndDebugLabel_DoesNotCrash) {
    if (HasFatalFailure()) return;
    auto& cmd = GetCommandBuffer();

    cmd.BeginDebugLabel("VKTestRegion", {1.0f, 0.5f, 0.2f, 1.0f});
    cmd.EndDebugLabel();

    // Just verify no crash - debug messages go to validation layers
    SUCCEED();
}

TEST_F(VKDebugLabelTest, NestedLabels_DoesNotCrash) {
    if (HasFatalFailure()) return;
    auto& cmd = GetCommandBuffer();

    cmd.BeginDebugLabel("Outer");
    cmd.BeginDebugLabel("Inner");
    cmd.EndDebugLabel();
    cmd.EndDebugLabel();

    SUCCEED();
}

TEST_F(VKDebugLabelTest, InsertDebugMarker_DoesNotCrash) {
    if (HasFatalFailure()) return;
    auto& cmd = GetCommandBuffer();

    cmd.InsertDebugMarker("MarkerA");
    cmd.InsertDebugMarker("MarkerB");
    cmd.InsertDebugMarker("MarkerC");

    SUCCEED();
}

TEST_F(VKDebugLabelTest, MultipleMarkersAndLabels_DoesNotCrash) {
    if (HasFatalFailure()) return;
    auto& cmd = GetCommandBuffer();

    cmd.BeginDebugLabel("Pass");
    cmd.InsertDebugMarker("Before");
    cmd.InsertDebugMarker("After");
    cmd.EndDebugLabel();

    SUCCEED();
}

TEST_F(VKDebugLabelTest, EmptyLabel_DoesNotCrash) {
    if (HasFatalFailure()) return;
    auto& cmd = GetCommandBuffer();

    cmd.BeginDebugLabel("");
    cmd.EndDebugLabel();

    SUCCEED();
}

} // namespace test
} // namespace rhi
