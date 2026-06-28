// ============================================================
// GL Draw Tests (requires GPU + OpenGL 4.6)
// Tests: Draw, DrawIndexed, DrawIndirect
// Verifies output via pixel readback and comparison.
// ============================================================

#include "GLTestFixture.hpp"
#include <glad/glad.h>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <fstream>

namespace rhi {
namespace test {

// ============================================================
// Pixel helpers
// ============================================================

struct PosColorVertex {
    float pos[3];
    float color[3];
};

static bool IsPixelColor(const uint8_t* pixel, uint8_t r, uint8_t g, uint8_t b, uint8_t a,
                         int tolerance = 2) {
    return std::abs(static_cast<int>(pixel[0]) - r) <= tolerance &&
           std::abs(static_cast<int>(pixel[1]) - g) <= tolerance &&
           std::abs(static_cast<int>(pixel[2]) - b) <= tolerance &&
           std::abs(static_cast<int>(pixel[3]) - a) <= tolerance;
}

static void SavePPM(const char* path, const uint8_t* pixels, uint32_t w, uint32_t h) {
    std::ofstream f(path, std::ios::binary);
    if (!f) return;
    f << "P6\n" << w << " " << h << "\n255\n";
    for (uint32_t y = 0; y < h; ++y) {
        for (uint32_t x = 0; x < w; ++x) {
            uint32_t idx = (y * w + x) * 4;
            f.put(static_cast<char>(pixels[idx]));
            f.put(static_cast<char>(pixels[idx + 1]));
            f.put(static_cast<char>(pixels[idx + 2]));
        }
    }
}

// ============================================================
// GLDrawTest
// ============================================================

class GLDrawTest : public GLTestFixture {
protected:
    void SetUp() override {
        GLTestFixture::SetUp();
        if (HasFatalFailure()) return;

        m_VS = LoadShaderFile(ShaderStage::Vertex, "tests/gl/shaders/simple.vert");
        m_FS_Color = LoadShaderFile(ShaderStage::Fragment, "tests/gl/shaders/color.frag");
        m_FS_Red = LoadShaderFile(ShaderStage::Fragment, "tests/gl/shaders/solid_red.frag");
        m_FS_Green = LoadShaderFile(ShaderStage::Fragment, "tests/gl/shaders/solid_green.frag");

        if (!m_VS.IsValid() || !m_FS_Color.IsValid()) {
            ADD_FAILURE() << "Shader files not found. Ensure working directory is VulkittenRHI root.";
            return;
        }

        m_RenderPass = CreateSimpleRenderPass(Format::RGBA8_UNORM);

        std::vector<VertexAttribute> layout = {
            {0, Format::RGB32_FLOAT, 0,  0, sizeof(PosColorVertex)},
            {1, Format::RGB32_FLOAT, 12, 0, sizeof(PosColorVertex)},
        };
        m_Pipeline = CreateGraphicsPipeline(m_VS, m_FS_Color, layout, m_RenderPass);
        if (!m_Pipeline.IsValid()) {
            ADD_FAILURE() << "Failed to create graphics pipeline";
        }
    }

    void BeginTestRenderPass() {
        auto& cmd = GetCommandBuffer();
        Rect2D area = {{0, 0}, {kTestWidth, kTestHeight}};
        ClearValue cv;
        cv.Color.RGBA = {0.0f, 0.0f, 0.0f, 0.0f};
        cmd.BeginRenderPass(m_RenderPass, {}, area, &cv, 1);
    }

    void EndTestRenderPass() {
        GetCommandBuffer().EndRenderPass();
    }

    ShaderHandle m_VS, m_FS_Color, m_FS_Red, m_FS_Green;
    RenderPassHandle m_RenderPass;
    PipelineHandle m_Pipeline;
};

// ============================================================
// Diagnostic: verify the GL context renders via raw OpenGL
// ============================================================

TEST_F(GLDrawTest, RawGL_ClearAndReadback) {
    // Bypass RHI entirely — verify GL context + default FB work
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f); // red
    glClear(GL_COLOR_BUFFER_BIT);
    glFinish();

    auto pixels = ReadAllPixels();
    ASSERT_EQ(pixels.size(), kTestWidth * kTestHeight * 4);

    // Every pixel should be red (255,0,0,255)
    const uint8_t* p = pixels.data();
    for (size_t i = 0; i < pixels.size(); i += 4) {
        ASSERT_EQ(p[i], 255u) << "R at pixel " << (i/4);
        ASSERT_EQ(p[i+1], 0u) << "G at pixel " << (i/4);
        ASSERT_EQ(p[i+2], 0u) << "B at pixel " << (i/4);
        ASSERT_EQ(p[i+3], 255u) << "A at pixel " << (i/4);
    }
}

// ============================================================
// Draw tests
// ============================================================

TEST_F(GLDrawTest, Draw_Triangle_CoversScreen) {
    if (HasFatalFailure()) return;
    auto& cmd = GetCommandBuffer();

    PosColorVertex vertices[] = {
        {{-1.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{ 3.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{-1.0f,  3.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    };

    BufferHandle vb = CreateVertexBuffer(vertices, sizeof(vertices));
    ASSERT_TRUE(vb.IsValid());
    GeometryHandle geo = CreateSimpleGeometry(vb, 3, sizeof(PosColorVertex));
    ASSERT_TRUE(geo.IsValid());

    BeginTestRenderPass();
    cmd.BindPipeline(m_Pipeline);
    cmd.BindGeometry(geo);
    cmd.Draw(3);
    EndTestRenderPass();

    glFinish();
    auto pixels = ReadAllPixels();
    ASSERT_EQ(pixels.size(), kTestWidth * kTestHeight * 4);

    SavePPM("test_output_Draw_Triangle.ppm", pixels.data(), kTestWidth, kTestHeight);

    // Center pixel should NOT be transparent black
    uint32_t cx = kTestWidth / 2;
    uint32_t cy = kTestHeight / 2;
    const uint8_t* center = &pixels[(cy * kTestWidth + cx) * 4];
    EXPECT_FALSE(IsPixelColor(center, 0, 0, 0, 0))
        << "Center pixel is black - triangle did not cover viewport";
}

TEST_F(GLDrawTest, Draw_FirstVertex_SkipsLeadingVertices) {
    if (HasFatalFailure()) return;
    auto& cmd = GetCommandBuffer();

    PosColorVertex vertices[] = {
        // First triangle (skipped by firstVertex=3)
        {{-1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
        {{ 0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
        {{-1.0f,  1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
        // Second triangle (red, should be drawn)
        {{ 0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{ 1.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.0f,  1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    };

    BufferHandle vb = CreateVertexBuffer(vertices, sizeof(vertices));
    ASSERT_TRUE(vb.IsValid());
    GeometryHandle geo = CreateSimpleGeometry(vb, 6, sizeof(PosColorVertex));
    ASSERT_TRUE(geo.IsValid());

    BeginTestRenderPass();
    cmd.BindPipeline(m_Pipeline);
    cmd.BindGeometry(geo);
    cmd.Draw(3, 3); // vertexCount=3, firstVertex=3
    EndTestRenderPass();

    glFinish();
    auto pixels = ReadAllPixels();

    // Second triangle covers x ∈ [0, 0.5] at y=0 (NDC). Column 40 (x≈0.28) is inside.
    uint32_t testX = kTestWidth * 5 / 8; // column 40
    uint32_t testY = kTestHeight / 2;    // center row
    const uint8_t* rightMid = &pixels[(testY * kTestWidth + testX) * 4];
    EXPECT_FALSE(IsPixelColor(rightMid, 0, 0, 0, 0))
        << "firstVertex=3: pixel at (" << testX << "," << testY << ") is black";
}

TEST_F(GLDrawTest, Draw_InstanceCount_TwoInstances) {
    if (HasFatalFailure()) return;
    auto& cmd = GetCommandBuffer();

    PosColorVertex vertices[] = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.0f,  0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    };

    BufferHandle vb = CreateVertexBuffer(vertices, sizeof(vertices));
    ASSERT_TRUE(vb.IsValid());
    GeometryHandle geo = CreateSimpleGeometry(vb, 3, sizeof(PosColorVertex));
    ASSERT_TRUE(geo.IsValid());

    BeginTestRenderPass();
    cmd.BindPipeline(m_Pipeline);
    cmd.BindGeometry(geo);
    cmd.Draw(3, 0, 2); // 2 instances
    EndTestRenderPass();

    glFinish();
    auto pixels = ReadAllPixels();

    // Center should not be black
    uint32_t cx = kTestWidth / 2;
    uint32_t cy = kTestHeight / 2;
    const uint8_t* center = &pixels[(cy * kTestWidth + cx) * 4];
    EXPECT_FALSE(IsPixelColor(center, 0, 0, 0, 0))
        << "Instanced draw produced no output at center";
}

// ============================================================
// DrawIndirect tests
// ============================================================

TEST_F(GLDrawTest, DrawIndirect_RendersTriangle) {
    if (HasFatalFailure()) return;
    auto& cmd = GetCommandBuffer();

    PosColorVertex vertices[] = {
        {{-1.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{ 3.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{-1.0f,  3.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    };

    BufferHandle vb = CreateVertexBuffer(vertices, sizeof(vertices));
    ASSERT_TRUE(vb.IsValid());
    GeometryHandle geo = CreateSimpleGeometry(vb, 3, sizeof(PosColorVertex));
    ASSERT_TRUE(geo.IsValid());

    // DrawArraysIndirectCommand: {vertexCount, instanceCount, firstVertex, firstInstance}
    std::vector<uint32_t> drawCmd = {3, 1, 0, 0};
    BufferHandle indirectBuf = CreateStorageBuffer(
        drawCmd.size() * sizeof(uint32_t), drawCmd.data());
    ASSERT_TRUE(indirectBuf.IsValid());

    BeginTestRenderPass();
    cmd.BindPipeline(m_Pipeline);
    cmd.BindGeometry(geo);
    cmd.DrawIndirect(indirectBuf, 0, 1, 16); // 1 draw, stride=16 bytes (4 u32s)
    EndTestRenderPass();

    glFinish();
    auto pixels = ReadAllPixels();
    SavePPM("test_output_DrawIndirect.ppm", pixels.data(), kTestWidth, kTestHeight);

    uint32_t cx = kTestWidth / 2;
    uint32_t cy = kTestHeight / 2;
    const uint8_t* center = &pixels[(cy * kTestWidth + cx) * 4];
    EXPECT_FALSE(IsPixelColor(center, 0, 0, 0, 0))
        << "DrawIndirect: center pixel is black";
}

} // namespace test
} // namespace rhi
