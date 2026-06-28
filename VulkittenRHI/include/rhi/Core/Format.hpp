#pragma once

#include <cstdint>

namespace rhi {

// ============================================================
// Format - cross-API pixel / vertex / depth-stencil format
//
// Naming: [Component][BitsPerChannel]_[Type]
//   Type: UNORM, FLOAT, UINT, SINT, SRGB
//
// Maps to: Vulkan VkFormat, OpenGL GLenum, DXGI_FORMAT, MTLPixelFormat
// ============================================================

enum class Format : uint32_t
{
    Unknown,

    // ---- 8-bit UNORM ----
    R8_UNORM,
    RG8_UNORM,
    RGBA8_UNORM,
    BGRA8_UNORM,
    RGBA8_SRGB,
    BGRA8_SRGB,

    // ---- 16-bit Float / UNORM ----
    R16_FLOAT,
    RG16_FLOAT,
    RGBA16_FLOAT,
    R16_UNORM,

    // ---- 32-bit Float ----
    R32_FLOAT,
    RG32_FLOAT,
    RGB32_FLOAT,
    RGBA32_FLOAT,

    // ---- 32-bit Integer ----
    R32_UINT,
    R32_SINT,
    RG32_UINT,
    RGBA32_UINT,

    // ---- Depth / Stencil ----
    D16_UNORM,
    D32_FLOAT,
    D24_UNORM_S8_UINT,
    D32_FLOAT_S8_UINT,

    // ---- BC Compressed ----
    BC1_RGB_UNORM,
    BC3_RGBA_UNORM,
    BC5_RG_UNORM,
    BC7_RGBA_UNORM,
};

// Byte size of a single pixel / texel block.
inline uint32_t FormatByteSize(Format f)
{
    switch (f)
    {
        case Format::R8_UNORM:                          return 1;
        case Format::RG8_UNORM:                         return 2;
        case Format::RGBA8_UNORM:
        case Format::BGRA8_UNORM:
        case Format::RGBA8_SRGB:
        case Format::BGRA8_SRGB:
        case Format::R32_FLOAT:
        case Format::R32_UINT:
        case Format::R32_SINT:                          return 4;
        case Format::RG16_FLOAT:                        return 4;
        case Format::RG32_FLOAT:
        case Format::RG32_UINT:                         return 8;
        case Format::RGB32_FLOAT:                       return 12;
        case Format::RGBA16_FLOAT:                      return 8;
        case Format::RGBA32_FLOAT:
        case Format::RGBA32_UINT:                       return 16;
        case Format::R16_FLOAT:
        case Format::R16_UNORM:
        case Format::D16_UNORM:                         return 2;
        case Format::D32_FLOAT:                         return 4;
        case Format::D24_UNORM_S8_UINT:
        case Format::D32_FLOAT_S8_UINT:                 return 4;
        case Format::BC1_RGB_UNORM:                     return 8;
        case Format::BC3_RGBA_UNORM:
        case Format::BC5_RG_UNORM:
        case Format::BC7_RGBA_UNORM:                    return 16;
        default:                                        return 0;
    }
}

// Number of components in a single pixel.
inline uint32_t FormatComponentCount(Format f)
{
    switch (f)
    {
        case Format::R8_UNORM:
        case Format::R16_FLOAT:
        case Format::R16_UNORM:
        case Format::R32_FLOAT:
        case Format::R32_UINT:
        case Format::R32_SINT:
        case Format::D16_UNORM:
        case Format::D32_FLOAT:                         return 1;
        case Format::RG8_UNORM:
        case Format::RG16_FLOAT:
        case Format::RG32_FLOAT:
        case Format::RG32_UINT:
        case Format::BC5_RG_UNORM:                      return 2;
        case Format::RGB32_FLOAT:                       return 3;
        case Format::RGBA8_UNORM:
        case Format::BGRA8_UNORM:
        case Format::RGBA8_SRGB:
        case Format::BGRA8_SRGB:
        case Format::RGBA16_FLOAT:
        case Format::RGBA32_FLOAT:
        case Format::RGBA32_UINT:
        case Format::BC1_RGB_UNORM:
        case Format::BC3_RGBA_UNORM:
        case Format::BC7_RGBA_UNORM:                    return 4;
        case Format::D24_UNORM_S8_UINT:
        case Format::D32_FLOAT_S8_UINT:                 return 2;
        default:                                        return 0;
    }
}

inline bool IsDepthFormat(Format f)
{
    return f == Format::D16_UNORM
        || f == Format::D32_FLOAT
        || f == Format::D24_UNORM_S8_UINT
        || f == Format::D32_FLOAT_S8_UINT;
}

inline bool IsStencilFormat(Format f)
{
    return f == Format::D24_UNORM_S8_UINT
        || f == Format::D32_FLOAT_S8_UINT;
}

inline bool IsCompressedFormat(Format f)
{
    return f == Format::BC1_RGB_UNORM
        || f == Format::BC3_RGBA_UNORM
        || f == Format::BC5_RG_UNORM
        || f == Format::BC7_RGBA_UNORM;
}

} // namespace rhi
