// ============================================================
// Format Unit Tests
//
// Tests for Format enum and helper functions.
// ============================================================

#include <gtest/gtest.h>
#include "rhi/Core/Format.hpp"

using namespace rhi;

TEST(FormatTest, FormatByteSize_CommonFormats)
{
    EXPECT_EQ(FormatByteSize(Format::R8_UNORM), 1u);
    EXPECT_EQ(FormatByteSize(Format::RG8_UNORM), 2u);
    EXPECT_EQ(FormatByteSize(Format::RGBA8_UNORM), 4u);
    EXPECT_EQ(FormatByteSize(Format::BGRA8_UNORM), 4u);
    EXPECT_EQ(FormatByteSize(Format::R32_FLOAT), 4u);
    EXPECT_EQ(FormatByteSize(Format::RG32_FLOAT), 8u);
    EXPECT_EQ(FormatByteSize(Format::RGB32_FLOAT), 12u);
    EXPECT_EQ(FormatByteSize(Format::RGBA32_FLOAT), 16u);
    EXPECT_EQ(FormatByteSize(Format::R32_UINT), 4u);
    EXPECT_EQ(FormatByteSize(Format::RGBA32_UINT), 16u);
}

TEST(FormatTest, FormatByteSize_Unknown)
{
    EXPECT_EQ(FormatByteSize(Format::Unknown), 0u);
}

TEST(FormatTest, FormatComponentCount_CommonFormats)
{
    EXPECT_EQ(FormatComponentCount(Format::R8_UNORM), 1u);
    EXPECT_EQ(FormatComponentCount(Format::R32_FLOAT), 1u);
    EXPECT_EQ(FormatComponentCount(Format::RG8_UNORM), 2u);
    EXPECT_EQ(FormatComponentCount(Format::RG32_FLOAT), 2u);
    EXPECT_EQ(FormatComponentCount(Format::RGB32_FLOAT), 3u);
    EXPECT_EQ(FormatComponentCount(Format::RGBA8_UNORM), 4u);
    EXPECT_EQ(FormatComponentCount(Format::BGRA8_UNORM), 4u);
    EXPECT_EQ(FormatComponentCount(Format::RGBA32_FLOAT), 4u);
}

TEST(FormatTest, FormatComponentCount_DepthFormats)
{
    EXPECT_EQ(FormatComponentCount(Format::D16_UNORM), 1u);
    EXPECT_EQ(FormatComponentCount(Format::D32_FLOAT), 1u);
    EXPECT_EQ(FormatComponentCount(Format::D24_UNORM_S8_UINT), 2u);
    EXPECT_EQ(FormatComponentCount(Format::D32_FLOAT_S8_UINT), 2u);
}

TEST(FormatTest, IsDepthFormat)
{
    EXPECT_TRUE(IsDepthFormat(Format::D16_UNORM));
    EXPECT_TRUE(IsDepthFormat(Format::D32_FLOAT));
    EXPECT_TRUE(IsDepthFormat(Format::D24_UNORM_S8_UINT));
    EXPECT_TRUE(IsDepthFormat(Format::D32_FLOAT_S8_UINT));

    EXPECT_FALSE(IsDepthFormat(Format::RGBA8_UNORM));
    EXPECT_FALSE(IsDepthFormat(Format::Unknown));
    EXPECT_FALSE(IsDepthFormat(Format::R32_FLOAT));
}

TEST(FormatTest, IsStencilFormat)
{
    EXPECT_TRUE(IsStencilFormat(Format::D24_UNORM_S8_UINT));
    EXPECT_TRUE(IsStencilFormat(Format::D32_FLOAT_S8_UINT));

    EXPECT_FALSE(IsStencilFormat(Format::D16_UNORM));
    EXPECT_FALSE(IsStencilFormat(Format::D32_FLOAT));
    EXPECT_FALSE(IsStencilFormat(Format::RGBA8_UNORM));
}

TEST(FormatTest, IsCompressedFormat)
{
    EXPECT_TRUE(IsCompressedFormat(Format::BC1_RGB_UNORM));
    EXPECT_TRUE(IsCompressedFormat(Format::BC3_RGBA_UNORM));
    EXPECT_TRUE(IsCompressedFormat(Format::BC5_RG_UNORM));
    EXPECT_TRUE(IsCompressedFormat(Format::BC7_RGBA_UNORM));

    EXPECT_FALSE(IsCompressedFormat(Format::RGBA8_UNORM));
    EXPECT_FALSE(IsCompressedFormat(Format::Unknown));
}

TEST(FormatTest, MultiFormatConsistency)
{
    // Each format should have consistent byte size and component count
    Format formats[] = {
        Format::R8_UNORM, Format::RG8_UNORM, Format::RGBA8_UNORM,
        Format::BGRA8_UNORM, Format::RGBA8_SRGB, Format::BGRA8_SRGB,
        Format::R32_FLOAT, Format::RG32_FLOAT, Format::RGB32_FLOAT,
        Format::RGBA32_FLOAT, Format::R32_UINT, Format::R32_SINT,
        Format::D16_UNORM, Format::D32_FLOAT,
    };

    for (auto fmt : formats)
    {
        uint32_t bs = FormatByteSize(fmt);
        uint32_t cc = FormatComponentCount(fmt);
        // Byte size should be at least component count
        EXPECT_GE(bs, cc) << "Format " << static_cast<uint32_t>(fmt)
                           << " has byte size " << bs << " < component count " << cc;
    }
}
