#pragma once

#include <cstdint>
#include <array>

namespace rhi {

// ============================================================
// Extents & Offsets
// ============================================================

struct Extent2D
{
    uint32_t Width = 0;
    uint32_t Height = 0;
};

struct Extent3D
{
    uint32_t Width = 0;
    uint32_t Height = 0;
    uint32_t Depth = 1;
};

struct Offset2D
{
    int32_t X = 0;
    int32_t Y = 0;
};

struct Offset3D
{
    int32_t X = 0;
    int32_t Y = 0;
    int32_t Z = 0;
};

struct Rect2D
{
    Offset2D Offset;
    Extent2D Extent;
};

// ============================================================
// Viewport
// ============================================================

struct Viewport
{
    float X = 0.0f;
    float Y = 0.0f;
    float Width = 0.0f;
    float Height = 0.0f;
    float MinDepth = 0.0f;
    float MaxDepth = 1.0f;
};

// ============================================================
// Clear Values
// ============================================================

struct ClearColor
{
    std::array<float, 4> RGBA = {0.0f, 0.0f, 0.0f, 1.0f};
};

struct ClearDepthStencil
{
    float Depth = 1.0f;
    uint32_t Stencil = 0;
};

union ClearValue
{
    ClearColor Color;
    ClearDepthStencil DepthStencil;

    ClearValue() : DepthStencil{} {}
};

// ============================================================
// Shader Stage (bitflag)
// ============================================================

enum class ShaderStage : uint32_t
{
    Vertex   = 1 << 0,
    Fragment = 1 << 1,
    Compute  = 1 << 2,
    RayGen   = 1 << 3,      // [RESERVE: RayTracing]
    Miss     = 1 << 4,      // [RESERVE: RayTracing]
    ClosestHit = 1 << 5,    // [RESERVE: RayTracing]
    Mesh     = 1 << 6,      // [RESERVE: MeshShader]
    Task     = 1 << 7,      // [RESERVE: MeshShader]
};

inline ShaderStage operator|(ShaderStage a, ShaderStage b)
{
    return static_cast<ShaderStage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline bool HasStage(ShaderStage flags, ShaderStage flag)
{
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
}

// ============================================================
// Index Type
// ============================================================

enum class IndexType : uint32_t
{
    UInt16,
    UInt32,
};

// ============================================================
// Buffer Usage (bitflag)
// ============================================================

enum class BufferUsage : uint32_t
{
    None        = 0,
    Vertex      = 1 << 0,
    Index       = 1 << 1,
    Uniform     = 1 << 2,
    Storage     = 1 << 3,
    Indirect    = 1 << 4,
    TransferSrc = 1 << 5,
    TransferDst = 1 << 6,
    AccelerationStructureInput = 1 << 7,  // [RESERVE: RayTracing]
    AccelerationStructureBuild = 1 << 8,  // [RESERVE: RayTracing]
};

inline BufferUsage operator|(BufferUsage a, BufferUsage b)
{
    return static_cast<BufferUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline bool HasUsage(BufferUsage flags, BufferUsage flag)
{
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
}

// ============================================================
// Texture Usage (bitflag)
// ============================================================

enum class TextureUsage : uint32_t
{
    None                    = 0,
    Sampled                 = 1 << 0,
    ColorAttachment         = 1 << 1,
    DepthStencilAttachment  = 1 << 2,
    Storage                 = 1 << 3,
    TransferSrc             = 1 << 4,
    TransferDst             = 1 << 5,
};

inline TextureUsage operator|(TextureUsage a, TextureUsage b)
{
    return static_cast<TextureUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline bool HasUsage(TextureUsage flags, TextureUsage flag)
{
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
}

// ============================================================
// Memory Property (bitflag)
// ============================================================

enum class MemoryProperty : uint32_t
{
    DeviceLocal  = 1 << 0,
    HostVisible  = 1 << 1,
    HostCoherent = 1 << 2,
    HostCached   = 1 << 3,
};

inline MemoryProperty operator|(MemoryProperty a, MemoryProperty b)
{
    return static_cast<MemoryProperty>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

// ============================================================
// Sampler State
// ============================================================

enum class FilterMode : uint32_t
{
    Nearest,
    Linear,
};

enum class MipMode : uint32_t
{
    Nearest,
    Linear,
};

enum class WrapMode : uint32_t
{
    Repeat,
    ClampToEdge,
    ClampToBorder,
    MirroredRepeat,
};

// ============================================================
// Pipeline Stage (bitflag, for barriers)
// ============================================================

enum class PipelineStage : uint32_t
{
    TopOfPipe             = 1 << 0,
    DrawIndirect          = 1 << 1,
    VertexInput           = 1 << 2,
    VertexShader          = 1 << 3,
    FragmentShader        = 1 << 4,
    EarlyFragmentTests    = 1 << 5,
    LateFragmentTests     = 1 << 6,
    ColorAttachmentOutput = 1 << 7,
    ComputeShader         = 1 << 8,
    Transfer              = 1 << 9,
    BottomOfPipe          = 1 << 10,
    Host                  = 1 << 11,
    RayTracingShader      = 1 << 12,  // [RESERVE: RayTracing]
};

inline PipelineStage operator|(PipelineStage a, PipelineStage b)
{
    return static_cast<PipelineStage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

// ============================================================
// Access Flags (bitflag, for barriers)
// ============================================================

enum class AccessFlags : uint32_t
{
    None                       = 0,
    IndirectCommandRead        = 1 << 0,
    IndexRead                  = 1 << 1,
    VertexAttributeRead        = 1 << 2,
    UniformRead                = 1 << 3,
    ShaderRead                 = 1 << 4,
    ShaderWrite                = 1 << 5,
    ColorAttachmentRead        = 1 << 6,
    ColorAttachmentWrite       = 1 << 7,
    DepthStencilAttachmentRead = 1 << 8,
    DepthStencilAttachmentWrite = 1 << 9,
    TransferRead               = 1 << 10,
    TransferWrite              = 1 << 11,
    MemoryRead                 = 1 << 12,
    MemoryWrite                = 1 << 13,
    AccelerationStructureRead  = 1 << 14,  // [RESERVE: RayTracing]
    AccelerationStructureWrite = 1 << 15,  // [RESERVE: RayTracing]
};

inline AccessFlags operator|(AccessFlags a, AccessFlags b)
{
    return static_cast<AccessFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

// ============================================================
// Image Layout (for texture barriers)
// ============================================================

enum class ImageLayout : uint32_t
{
    Undefined,
    General,
    ColorAttachment,
    DepthStencilAttachment,
    DepthStencilReadOnly,
    ShaderRead,
    TransferSrc,
    TransferDst,
    PresentSrc,
    AccelerationStructure,  // [RESERVE: RayTracing]
};

// ============================================================
// RenderPass Attachment Operations
// ============================================================

enum class LoadOp : uint32_t
{
    Load,
    Clear,
    DontCare,
};

enum class StoreOp : uint32_t
{
    Store,
    DontCare,
};

// ============================================================
// Query Type (for GPU timestamp / occlusion / statistics queries)
// ============================================================

enum class QueryType : uint32_t
{
    Timestamp,
    Occlusion,
    PipelineStatistics,
};

// ============================================================
// Command Buffer Level  [RESERVE: Multi-threaded recording]
// ============================================================

enum class CommandBufferLevel : uint32_t
{
    Primary,
    Secondary,
};

// ============================================================
// Queue Type  [RESERVE: AsyncCompute]
// ============================================================

enum class QueueType : uint32_t
{
    Graphics,
    Compute,
    Transfer,
};

} // namespace rhi
