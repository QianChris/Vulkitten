// ============================================================
// GL Debug Label Tests (requires GPU + OpenGL 4.6 + GL_KHR_debug)
// Tests: BeginDebugLabel, EndDebugLabel, InsertDebugMarker
// Verifies debug output via GL debug message callback capture.
// ============================================================

#include "GLTestFixture.hpp"
#include <glad/glad.h>
#include <cstring>
#include <cstdio>
#include <algorithm>

namespace rhi {
namespace test {

class GLDebugLabelTest : public GLTestFixture {
protected:
    void SetUp() override {
        GLTestFixture::SetUp();
        if (HasFatalFailure()) return;
        BeginDebugCapture();
    }

    void TearDown() override {
        EndDebugCapture();
        GLTestFixture::TearDown();
    }

    bool ContainsText(const char* text) {
        auto& msgs = GetCapturedDebugMessages();
        for (auto& m : msgs) {
            if (m.find(text) != std::string::npos)
                return true;
        }
        return false;
    }

    void PrintAll() {
        auto& msgs = GetCapturedDebugMessages();
        printf("=== %zu debug messages ===\n", msgs.size());
        for (auto& m : msgs) printf("  %s\n", m.c_str());
    }
};

TEST_F(GLDebugLabelTest, BeginEndDebugLabel_ProducesPushPop) {
    if (HasFatalFailure()) return;
    auto& cmd = GetCommandBuffer();

    ClearDebugMessages();
    cmd.BeginDebugLabel("TestRegion", {1.0f, 0.5f, 0.0f, 1.0f});
    cmd.EndDebugLabel();
    glFinish();
    PrintAll();

    auto& msgs = GetCapturedDebugMessages();
    EXPECT_GT(msgs.size(), 0u) << "No debug messages received";

    bool foundPush = false, foundPop = false;
    for (auto& m : msgs) {
        if (m.find("PUSH_GROUP") != std::string::npos &&
            m.find("TestRegion") != std::string::npos)
            foundPush = true;
        if (m.find("POP_GROUP") != std::string::npos)
            foundPop = true;
    }
    EXPECT_TRUE(foundPush) << "PUSH_GROUP for 'TestRegion' not found";
    EXPECT_TRUE(foundPop) << "POP_GROUP not found";
}

TEST_F(GLDebugLabelTest, NestedLabels) {
    if (HasFatalFailure()) return;
    auto& cmd = GetCommandBuffer();

    ClearDebugMessages();
    cmd.BeginDebugLabel("Outer");
    cmd.BeginDebugLabel("Inner");
    cmd.EndDebugLabel();
    cmd.EndDebugLabel();
    glFinish();
    PrintAll();

    EXPECT_TRUE(ContainsText("Outer")) << "Outer not found";
    EXPECT_TRUE(ContainsText("Inner")) << "Inner not found";
}

TEST_F(GLDebugLabelTest, InsertDebugMarker_AppearsAsMarker) {
    if (HasFatalFailure()) return;
    auto& cmd = GetCommandBuffer();

    ClearDebugMessages();
    cmd.InsertDebugMarker("MarkerPoint");
    glFinish();
    PrintAll();

    auto& msgs = GetCapturedDebugMessages();
    bool found = false;
    for (auto& m : msgs) {
        if (m.find("MARKER") != std::string::npos &&
            m.find("MarkerPoint") != std::string::npos)
            found = true;
    }
    EXPECT_TRUE(found) << "MARKER for 'MarkerPoint' not found";
}

TEST_F(GLDebugLabelTest, MultipleMarkers) {
    if (HasFatalFailure()) return;
    auto& cmd = GetCommandBuffer();

    ClearDebugMessages();
    cmd.InsertDebugMarker("Marker_A");
    cmd.InsertDebugMarker("Marker_B");
    cmd.InsertDebugMarker("Marker_C");
    glFinish();
    PrintAll();

    EXPECT_TRUE(ContainsText("Marker_A"));
    EXPECT_TRUE(ContainsText("Marker_B"));
    EXPECT_TRUE(ContainsText("Marker_C"));
}

TEST_F(GLDebugLabelTest, EmptyLabel_DoesNotCrash) {
    if (HasFatalFailure()) return;
    auto& cmd = GetCommandBuffer();

    ClearDebugMessages();
    cmd.BeginDebugLabel("");
    cmd.EndDebugLabel();
    glFinish();
    SUCCEED(); // Just verify no crash
}

TEST_F(GLDebugLabelTest, UnmatchedEnd_DoesNotCrash) {
    if (HasFatalFailure()) return;
    auto& cmd = GetCommandBuffer();

    ClearDebugMessages();
    cmd.EndDebugLabel(); // No matching begin
    glFinish();
    SUCCEED();
}

} // namespace test
} // namespace rhi
