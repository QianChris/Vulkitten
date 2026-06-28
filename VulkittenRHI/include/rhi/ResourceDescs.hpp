#pragma once

#include "rhi/Core/Handle.hpp"
#include "rhi/Core/Format.hpp"
#include "rhi/Core/Types.hpp"

#include <vector>
#include <array>

namespace rhi {

// ============================================================
// TextureType
// ============================================================

enum class TextureType : uint32_t
{
    Texture1D,
    Texture2D,
    Texture3D,
    TextureCube,
    Texture2DArray,
};

// ============================================================
// BufferDesc
// ============================================================

struct BufferDesc
{
    uint64_t       Size = 0;
    BufferUsage    Usage = BufferUsage::None;
    MemoryProperty Memory = MemoryProperty::DeviceLocal;
    bool           CpuAccessible = false;
};

// ============================================================
// TextureDesc
// ============================================================

struct TextureDesc
{
    TextureType    Type = TextureType::Texture2D;
    Format         Format = Format::Unknown;
    Extent3D       Extent;
    uint32_t       MipLevels = 1;
    uint32_t       ArrayLayers = 1;
    TextureUsage   Usage = TextureUsage::Sampled;
    MemoryProperty Memory = MemoryProperty::DeviceLocal;
    bool           CpuAccessible = false;
};

// ============================================================
// TextureViewDesc
// ============================================================

struct TextureViewDesc
{
    TextureHandle Source;
    Format        Format = Format::Unknown;
    uint32_t      BaseMipLevel = 0;
    uint32_t      MipLevelCount = 1;
    uint32_t      BaseArrayLayer = 0;
    uint32_t      ArrayLayerCount = 1;
};

// ============================================================
// VertexAttribute - per-attribute layout
//
// KEY: Vertex format lives in Pipeline, NOT in Geometry.
// ============================================================

struct VertexAttribute
{
    uint32_t Location = 0;
    Format   Format = Format::Unknown;
    uint32_t Offset = 0;
    uint32_t BufferSlot = 0;
    uint32_t Stride = 0;
};

// ============================================================
// RasterState
// ============================================================

struct RasterState
{
    enum class CullMode { None, Front, Back };
    enum class FrontFace { CounterClockwise, Clockwise };
    enum class PolygonMode { Fill, Line, Point };

    CullMode    Cull              = CullMode::Back;
    FrontFace   Front             = FrontFace::CounterClockwise;
    PolygonMode Poly              = PolygonMode::Fill;
    bool        DepthClampEnable  = false;
    bool        ScissorEnable     = false;
};

// ============================================================
// DepthStencilState
// ============================================================

struct DepthStencilState
{
    enum class CompareOp
    {
        Never, Less, Equal, LessEqual,
        Greater, NotEqual, GreaterEqual, Always,
    };

    bool      DepthTestEnable   = true;
    bool      DepthWriteEnable  = true;
    CompareOp DepthCompareOp    = CompareOp::Less;
    bool      StencilTestEnable = false;
};

// ============================================================
// BlendState - per color attachment
// ============================================================

struct BlendState
{
    enum class BlendFactor
    {
        Zero, One, SrcColor, OneMinusSrcColor,
        SrcAlpha, OneMinusSrcAlpha, DstAlpha, DstColor,
    };
    enum class BlendOp { Add, Subtract, ReverseSubtract, Min, Max };

    bool       Enable           = false;
    BlendFactor SrcColorFactor  = BlendFactor::One;
    BlendFactor DstColorFactor  = BlendFactor::Zero;
    BlendOp    ColorOp          = BlendOp::Add;
    BlendFactor SrcAlphaFactor  = BlendFactor::One;
    BlendFactor DstAlphaFactor  = BlendFactor::Zero;
    BlendOp    AlphaOp          = BlendOp::Add;
    bool WriteR = true, WriteG = true, WriteB = true, WriteA = true;
};

// ============================================================
// TextureSlot - declares texture binding in pipeline
// ============================================================

struct TextureSlot
{
    enum class Type : uint32_t { Sampled, Storage };

    uint32_t    Slot = 0;
    Type        SlotType = Type::Sampled;
    ShaderStage Stages = ShaderStage::Fragment;
};

// ============================================================
// BufferSlot - declares buffer binding in pipeline
// ============================================================

struct BufferSlot
{
    enum class Type : uint32_t { Uniform, Storage, PushConstant };

    uint32_t    Slot = 0;
    Type        SlotType = Type::Uniform;
    ShaderStage Stages = ShaderStage::Vertex | ShaderStage::Fragment;
    uint32_t    Size = 0;
};

// ============================================================
// PipelineDesc
// ============================================================

struct PipelineDesc
{
    ShaderHandle VertexShader;
    ShaderHandle FragmentShader;
    ShaderHandle ComputeShader;    // If valid, compute pipeline (VS/FS ignored)

    std::vector<VertexAttribute> VertexLayout;
    RasterState       Raster;
    DepthStencilState DepthStencil;
    std::vector<BlendState> Blends;

    RenderPassHandle  RenderPass;
    uint32_t          SubpassIndex = 0;
    uint32_t          PushConstantsSize = 0;

    std::vector<TextureSlot> TextureSlots;
    std::vector<BufferSlot>  BufferSlots;
};

// ============================================================
// GeometryDesc - pure vertex/index buffer references
//
// KEY: Geometry carries NO vertex format information.
// Format is determined by PipelineDesc::VertexLayout.
// ============================================================

struct GeometryDesc
{
    static constexpr uint32_t MAX_VERTEX_BUFFERS = 8;

    std::array<BufferHandle, MAX_VERTEX_BUFFERS> VertexBuffers;
    uint32_t VertexBufferCount = 0;

    BufferHandle IndexBuffer;
    IndexType    IndexType = IndexType::UInt32;

    uint32_t VertexCount = 0;
    uint32_t IndexCount  = 0;
    uint32_t FirstVertex  = 0;
    uint32_t FirstIndex   = 0;
};

// ============================================================
// SamplerDesc
// ============================================================

struct SamplerDesc
{
    FilterMode MagFilter     = FilterMode::Linear;
    FilterMode MinFilter     = FilterMode::Linear;
    MipMode    Mip           = MipMode::Linear;
    WrapMode   WrapU         = WrapMode::Repeat;
    WrapMode   WrapV         = WrapMode::Repeat;
    WrapMode   WrapW         = WrapMode::Repeat;
    float      MaxAnisotropy = 1.0f;
};

// ============================================================
// AttachmentDesc - per-attachment RenderPass description
// ============================================================

struct AttachmentDesc
{
    Format      Format = Format::Unknown;
    uint32_t    Samples = 1;
    LoadOp      LoadOp = LoadOp::Clear;
    StoreOp     StoreOp = StoreOp::Store;
    ImageLayout InitialLayout = ImageLayout::Undefined;
    ImageLayout FinalLayout = ImageLayout::ColorAttachment;
    ClearValue  ClearValue = {};
};

// ============================================================
// SubpassDesc
// ============================================================

struct SubpassDesc
{
    std::vector<uint32_t> ColorAttachments;
    int32_t               DepthStencilAttachment = -1;
    std::vector<uint32_t> InputAttachments;
    // uint32_t ViewMask = 0;  // [RESERVE: Multiview]
};

// ============================================================
// SubpassDependency
// ============================================================

struct SubpassDependency
{
    uint32_t      SrcSubpass;   // ~0u = VK_SUBPASS_EXTERNAL
    uint32_t      DstSubpass;
    PipelineStage SrcStage;
    PipelineStage DstStage;
    AccessFlags   SrcAccess;
    AccessFlags   DstAccess;
};

// ============================================================
// RenderPassDesc
// ============================================================

struct RenderPassDesc
{
    std::vector<AttachmentDesc>  ColorAttachments;
    AttachmentDesc               DepthStencilAttachment;  // Format::Unknown = none
    std::vector<SubpassDesc>     Subpasses;
    std::vector<SubpassDependency> Dependencies;
};

// ============================================================
// FramebufferDesc
// ============================================================

struct FramebufferDesc
{
    RenderPassHandle          RenderPass;
    std::vector<TextureHandle> ColorAttachments;
    TextureHandle             DepthStencilAttachment;
    uint32_t                  Width = 0;
    uint32_t                  Height = 0;
    uint32_t                  Layers = 1;
};

// ============================================================
// QueryPoolDesc
// ============================================================

struct QueryPoolDesc
{
    QueryType Type = QueryType::Timestamp;
    uint32_t  Count = 2;  // typically 2 for timestamp (begin + end)
};

} // namespace rhi
