// ============================================================
// Core Types Unit Tests
//
// Tests for Extent, Offset, ClearValue, enums, and bitflags.
// ============================================================

#include <gtest/gtest.h>
#include "rhi/Core/Types.hpp"

using namespace rhi;

// ============================================================
// Extent2D / Extent3D
// ============================================================

TEST(TypesTest, Extent2D_DefaultConstruction)
{
    Extent2D e;
    EXPECT_EQ(e.Width, 0u);
    EXPECT_EQ(e.Height, 0u);
}

TEST(TypesTest, Extent2D_ValueConstruction)
{
    Extent2D e{1920, 1080};
    EXPECT_EQ(e.Width, 1920u);
    EXPECT_EQ(e.Height, 1080u);
}

TEST(TypesTest, Extent3D_DefaultDepth)
{
    Extent3D e;
    EXPECT_EQ(e.Width, 0u);
    EXPECT_EQ(e.Height, 0u);
    EXPECT_EQ(e.Depth, 1u); // default depth is 1
}

TEST(TypesTest, Extent3D_ValueConstruction)
{
    Extent3D e{256, 256, 16};
    EXPECT_EQ(e.Width, 256u);
    EXPECT_EQ(e.Height, 256u);
    EXPECT_EQ(e.Depth, 16u);
}

// ============================================================
// Offset2D / Offset3D
// ============================================================

TEST(TypesTest, Offset2D_DefaultConstruction)
{
    Offset2D o;
    EXPECT_EQ(o.X, 0);
    EXPECT_EQ(o.Y, 0);
}

TEST(TypesTest, Offset2D_NegativeValues)
{
    Offset2D o{-10, -20};
    EXPECT_EQ(o.X, -10);
    EXPECT_EQ(o.Y, -20);
}

TEST(TypesTest, Offset3D_DefaultConstruction)
{
    Offset3D o;
    EXPECT_EQ(o.X, 0);
    EXPECT_EQ(o.Y, 0);
    EXPECT_EQ(o.Z, 0);
}

// ============================================================
// Rect2D
// ============================================================

TEST(TypesTest, Rect2D_DefaultConstruction)
{
    Rect2D r;
    EXPECT_EQ(r.Offset.X, 0);
    EXPECT_EQ(r.Offset.Y, 0);
    EXPECT_EQ(r.Extent.Width, 0u);
    EXPECT_EQ(r.Extent.Height, 0u);
}

TEST(TypesTest, Rect2D_FullViewport)
{
    Rect2D r{{0, 0}, {1920, 1080}};
    EXPECT_EQ(r.Offset.X, 0);
    EXPECT_EQ(r.Offset.Y, 0);
    EXPECT_EQ(r.Extent.Width, 1920u);
    EXPECT_EQ(r.Extent.Height, 1080u);
}

// ============================================================
// Viewport
// ============================================================

TEST(TypesTest, Viewport_DefaultConstruction)
{
    Viewport vp;
    EXPECT_FLOAT_EQ(vp.X, 0.0f);
    EXPECT_FLOAT_EQ(vp.Y, 0.0f);
    EXPECT_FLOAT_EQ(vp.Width, 0.0f);
    EXPECT_FLOAT_EQ(vp.Height, 0.0f);
    EXPECT_FLOAT_EQ(vp.MinDepth, 0.0f);
    EXPECT_FLOAT_EQ(vp.MaxDepth, 1.0f);
}

// ============================================================
// ClearValue
// ============================================================

TEST(TypesTest, ClearValue_DefaultIsDepthStencil)
{
    ClearValue cv;
    EXPECT_FLOAT_EQ(cv.DepthStencil.Depth, 1.0f);
    EXPECT_EQ(cv.DepthStencil.Stencil, 0u);
}

TEST(TypesTest, ClearColor_Default)
{
    ClearColor cc;
    EXPECT_FLOAT_EQ(cc.RGBA[0], 0.0f);
    EXPECT_FLOAT_EQ(cc.RGBA[1], 0.0f);
    EXPECT_FLOAT_EQ(cc.RGBA[2], 0.0f);
    EXPECT_FLOAT_EQ(cc.RGBA[3], 1.0f);
}

TEST(TypesTest, ClearColor_CustomValues)
{
    ClearColor cc{{0.2f, 0.4f, 0.6f, 0.8f}};
    EXPECT_FLOAT_EQ(cc.RGBA[0], 0.2f);
    EXPECT_FLOAT_EQ(cc.RGBA[1], 0.4f);
    EXPECT_FLOAT_EQ(cc.RGBA[2], 0.6f);
    EXPECT_FLOAT_EQ(cc.RGBA[3], 0.8f);
}

// ============================================================
// ShaderStage bitflags
// ============================================================

TEST(TypesTest, ShaderStage_OrOperator)
{
    auto stages = ShaderStage::Vertex | ShaderStage::Fragment;
    EXPECT_TRUE(HasStage(stages, ShaderStage::Vertex));
    EXPECT_TRUE(HasStage(stages, ShaderStage::Fragment));
    EXPECT_FALSE(HasStage(stages, ShaderStage::Compute));
}

TEST(TypesTest, ShaderStage_SingleFlag)
{
    EXPECT_TRUE(HasStage(ShaderStage::Compute, ShaderStage::Compute));
    EXPECT_FALSE(HasStage(ShaderStage::Compute, ShaderStage::Vertex));
}

// ============================================================
// BufferUsage bitflags
// ============================================================

TEST(TypesTest, BufferUsage_OrOperator)
{
    auto usage = BufferUsage::Vertex | BufferUsage::Index;
    EXPECT_TRUE(HasUsage(usage, BufferUsage::Vertex));
    EXPECT_TRUE(HasUsage(usage, BufferUsage::Index));
    EXPECT_FALSE(HasUsage(usage, BufferUsage::Uniform));
}

TEST(TypesTest, BufferUsage_None)
{
    EXPECT_FALSE(HasUsage(BufferUsage::None, BufferUsage::Vertex));
    EXPECT_FALSE(HasUsage(BufferUsage::None, BufferUsage::Index));
}

// ============================================================
// MemoryProperty bitflags
// ============================================================

TEST(TypesTest, MemoryProperty_OrOperator)
{
    auto mem = MemoryProperty::HostVisible | MemoryProperty::HostCoherent;
    EXPECT_NE(static_cast<uint32_t>(mem), 0u);
}

// ============================================================
// PipelineStage bitflags
// ============================================================

TEST(TypesTest, PipelineStage_OrOperator)
{
    auto stage = PipelineStage::VertexShader | PipelineStage::FragmentShader;
    EXPECT_NE(static_cast<uint32_t>(stage), 0u);
}

// ============================================================
// AccessFlags bitflags
// ============================================================

TEST(TypesTest, AccessFlags_OrOperator)
{
    auto access = AccessFlags::ShaderRead | AccessFlags::ShaderWrite;
    EXPECT_NE(static_cast<uint32_t>(access), 0u);
}

TEST(TypesTest, AccessFlags_None)
{
    EXPECT_EQ(static_cast<uint32_t>(AccessFlags::None), 0u);
}

// ============================================================
// IndexType
// ============================================================

TEST(TypesTest, IndexType_Values)
{
    EXPECT_NE(static_cast<uint32_t>(IndexType::UInt16), static_cast<uint32_t>(IndexType::UInt32));
}

// ============================================================
// CommandBufferLevel
// ============================================================

TEST(TypesTest, CommandBufferLevel_Values)
{
    EXPECT_NE(static_cast<uint32_t>(CommandBufferLevel::Primary),
              static_cast<uint32_t>(CommandBufferLevel::Secondary));
}
