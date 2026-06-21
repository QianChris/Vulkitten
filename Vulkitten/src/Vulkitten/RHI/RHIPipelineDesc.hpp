#pragma once

#include "Vulkitten/RHI/Handle.hpp"
#include "Vulkitten/RHI/Core/Format.hpp"
#include "Vulkitten/RHI/Core/Types.hpp"

#include <vector>
#include <array>

namespace Vulkitten {
namespace rhi {

// ============================================================
// VertexAttribute — per-attribute layout in a vertex buffer
//
// KEY DESIGN: Vertex format lives in Pipeline, NOT in Geometry.
// A single Geometry can be rendered with different Pipelines
// that interpret the vertex data with different formats.
// ============================================================

struct VertexAttribute
{
    uint32_t Location = 0;       // Shader input location (layout(location=N))
    Format   Format = Format::Unknown;
    uint32_t Offset = 0;         // Byte offset within this buffer slot
    uint32_t BufferSlot = 0;     // Index into GeometryDesc::VertexBuffers[]
    uint32_t Stride = 0;         // Byte stride for this buffer slot
};

// ============================================================
// RasterState
// ============================================================

struct RasterState
{
    enum class CullMode
    {
        None,
        Front,
        Back,
    };

    enum class FrontFace
    {
        CounterClockwise,
        Clockwise,
    };

    enum class PolygonMode
    {
        Fill,
        Line,
        Point,
    };

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
        Never,
        Less,
        Equal,
        LessEqual,
        Greater,
        NotEqual,
        GreaterEqual,
        Always,
    };

    bool      DepthTestEnable   = true;
    bool      DepthWriteEnable  = true;
    CompareOp DepthCompareOp    = CompareOp::Less;

    bool      StencilTestEnable = false;
    // Full stencil front/back ops deferred — add when needed.
};

// ============================================================
// BlendState — per color attachment
// ============================================================

struct BlendState
{
    enum class BlendFactor
    {
        Zero,
        One,
        SrcColor,
        OneMinusSrcColor,
        SrcAlpha,
        OneMinusSrcAlpha,
        DstAlpha,
        DstColor,
    };

    enum class BlendOp
    {
        Add,
        Subtract,
        ReverseSubtract,
        Min,
        Max,
    };

    bool       Enable           = false;

    BlendFactor SrcColorFactor  = BlendFactor::One;
    BlendFactor DstColorFactor  = BlendFactor::Zero;
    BlendOp    ColorOp          = BlendOp::Add;

    BlendFactor SrcAlphaFactor  = BlendFactor::One;
    BlendFactor DstAlphaFactor  = BlendFactor::Zero;
    BlendOp    AlphaOp          = BlendOp::Add;

    bool WriteR = true;
    bool WriteG = true;
    bool WriteB = true;
    bool WriteA = true;
};

// ============================================================
// TextureSlot — declares a texture binding slot in the pipeline
//
// Tells the backend "this pipeline expects a texture at slot N,
// for use as Sampled or Storage, visible to these shader stages".
// Backends use this to pre-build descriptor layouts (Vulkan:
// VkDescriptorSetLayout; OpenGL: uniform location → texture
// unit mapping table).
// ============================================================

struct TextureSlot
{
    enum class Type : uint32_t
    {
        Sampled,   // Sampled image (fragment shader texture)
        Storage,   // Storage image (compute UAV / ImageStore)
    };

    uint32_t    Slot = 0;
    Type        SlotType = Type::Sampled;
    ShaderStage Stages = ShaderStage::Fragment;  // Which stages access this texture
};

// ============================================================
// BufferSlot — declares a buffer binding slot in the pipeline
//
// Tells the backend "this pipeline expects a buffer at slot N,
// used as Uniform, Storage, or PushConstant, with a given size
// hint, visible to these shader stages".
// ============================================================

struct BufferSlot
{
    enum class Type : uint32_t
    {
        Uniform,       // Uniform buffer (UBO)
        Storage,       // Storage buffer (SSBO)
        PushConstant,  // Push constant block
    };

    uint32_t    Slot = 0;
    Type        SlotType = Type::Uniform;
    ShaderStage Stages = ShaderStage::Vertex | ShaderStage::Fragment;
    uint32_t    Size = 0;   // Size in bytes (0 = unknown / runtime-determined)
};

// ============================================================
// PipelineDesc — complete graphics pipeline description
//
// For compute pipelines, set ComputeShader and leave VS/FS null.
// Vertex format is here (vertexLayout), NOT in GeometryDesc.
//
// TextureSlots and BufferSlots declare the binding interface of
// this pipeline. Backends read these at createPipeline() time to
// pre-build descriptor layouts. The slots used at bind time
// (ICommandBuffer::bindTexture/bindUniformBuffer etc.) must be
// a subset of what is declared here.
// ============================================================

struct PipelineDesc
{
    ShaderHandle VertexShader;
    ShaderHandle FragmentShader;
    ShaderHandle ComputeShader;    // If valid, this is a compute pipeline (VS/FS ignored)

    std::vector<VertexAttribute> VertexLayout;

    RasterState       Raster;
    DepthStencilState DepthStencil;
    std::vector<BlendState> Blends;       // One per color attachment

    RenderPassHandle  RenderPass;         // Compatible render pass (required for Vulkan)
    uint32_t          SubpassIndex = 0;

    uint32_t          PushConstantsSize = 0;  // Bytes of push constant data

    // ---- Slot Declarations ----
    // Backends use these at pipeline creation to build descriptor
    // layouts. Bind-time slots must be a subset of what's declared.
    std::vector<TextureSlot> TextureSlots;
    std::vector<BufferSlot>  BufferSlots;
};

// ============================================================
// GeometryDesc — pure vertex/index buffer references
//
// KEY DESIGN: Geometry carries NO vertex format information.
// The format is determined by the Pipeline that renders this
// geometry (via PipelineDesc::VertexLayout).
//
// Supports up to 8 vertex buffer streams (positions separate
// from UVs, etc.). Index buffer is optional (null = non-indexed).
// ============================================================

struct GeometryDesc
{
    static constexpr uint32_t MAX_VERTEX_BUFFERS = 8;

    std::array<BufferHandle, MAX_VERTEX_BUFFERS> VertexBuffers;
    uint32_t VertexBufferCount = 0;

    BufferHandle IndexBuffer;      // Null handle = non-indexed draw
    IndexType    IndexType = IndexType::UInt32;

    uint32_t VertexCount = 0;
    uint32_t IndexCount  = 0;      // 0 = non-indexed
    uint32_t FirstVertex  = 0;
    uint32_t FirstIndex   = 0;
};

// ============================================================
// SamplerDesc — texture sampling configuration
// ============================================================

struct SamplerDesc
{
    FilterMode MagFilter     = FilterMode::Linear;
    FilterMode MinFilter     = FilterMode::Linear;
    MipMode    Mip           = MipMode::Linear;
    WrapMode   WrapU         = WrapMode::Repeat;
    WrapMode   WrapV         = WrapMode::Repeat;
    WrapMode   WrapW         = WrapMode::Repeat;
    float      MaxAnisotropy = 1.0f;  // 1 = off
};

} // namespace rhi
} // namespace Vulkitten
