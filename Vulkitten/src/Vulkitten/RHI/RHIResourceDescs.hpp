#pragma once

#include "Vulkitten/RHI/Handle.hpp"
#include "Vulkitten/RHI/Core/Format.hpp"
#include "Vulkitten/RHI/Core/Types.hpp"

#include <vector>

namespace Vulkitten {
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
    bool           CpuAccessible = false;       // For staging / readback
};

// ============================================================
// TextureDesc
// ============================================================

struct TextureDesc
{
    TextureType    Type = TextureType::Texture2D;
    Format         Format = Format::Unknown;
    Extent3D       Extent;                       // width/height/depth
    uint32_t       MipLevels = 1;
    uint32_t       ArrayLayers = 1;              // Cube = 6, Array = N
    TextureUsage   Usage = TextureUsage::Sampled;
    MemoryProperty Memory = MemoryProperty::DeviceLocal;
    bool           CpuAccessible = false;
};

// ============================================================
// TextureViewDesc (optional, for aliasing / format reinterpret)
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
// AttachmentDesc - per-attachment description for RenderPass
// ============================================================

struct AttachmentDesc
{
    Format      Format = Format::Unknown;
    uint32_t    Samples = 1;                     // MSAA (start with 1)
    LoadOp      LoadOp = LoadOp::Clear;
    StoreOp     StoreOp = StoreOp::Store;
    ImageLayout InitialLayout = ImageLayout::Undefined;
    ImageLayout FinalLayout = ImageLayout::ColorAttachment;
    ClearValue  ClearValue = {};
};

// ============================================================
// SubpassDesc - a single subpass within a RenderPass
// ============================================================

struct SubpassDesc
{
    std::vector<uint32_t> ColorAttachments;        // Indices into RenderPassDesc::ColorAttachments
    int32_t               DepthStencilAttachment = -1;  // -1 = none
    std::vector<uint32_t> InputAttachments;        // For deferred rendering (future)
};

// ============================================================
// SubpassDependency - explicit subpass-to-subpass barrier
// ============================================================

struct SubpassDependency
{
    uint32_t      SrcSubpass;    // VK_SUBPASS_EXTERNAL = ~0u
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
    std::vector<SubpassDesc>     Subpasses;               // Start with 1
    std::vector<SubpassDependency> Dependencies;           // Can be empty
};

// ============================================================
// FramebufferDesc - binds textures to a RenderPass
// ============================================================

struct FramebufferDesc
{
    RenderPassHandle          RenderPass;                 // Compatible render pass
    std::vector<TextureHandle> ColorAttachments;
    TextureHandle             DepthStencilAttachment;     // Null = none
    uint32_t                  Width = 0;
    uint32_t                  Height = 0;
    uint32_t                  Layers = 1;
};

} // namespace rhi
} // namespace Vulkitten
