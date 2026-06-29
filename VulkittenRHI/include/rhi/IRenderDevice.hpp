#pragma once

#include "rhi/Core/Export.hpp"
#include "rhi/Core/Handle.hpp"
#include "rhi/Core/Types.hpp"
#include "rhi/ResourceDescs.hpp"
#include "rhi/FrameContext.hpp"
#include "rhi/ISurface.hpp"

#include <memory>

namespace rhi {

class ICommandBuffer;
class IBuffer;
class ITexture;
class IShader;
class IPipeline;
class IGeometry;
class ISampler;

// ============================================================
// ShaderBytecode - shader binary data + entry point
// ============================================================

struct ShaderBytecode
{
    const uint8_t* Data = nullptr;
    size_t         Size = 0;
    const char*    EntryPoint = "main";
};

// ============================================================
// DeviceConfig
// ============================================================

struct DeviceConfig
{
    ISurface*   Surface = nullptr;
    bool        EnableValidation = true;
    bool        EnableDebugMarkers = true;
    uint32_t    FramesInFlight = 2;
};

// ============================================================
// IRenderDevice - abstract GPU device interface
//
// Represents a logical GPU device. Owns the frame lifecycle
// (swapchain acquire → command recording → submit → present),
// creates all GPU resources, and provides command buffers.
//
// OpenGL:  the GL context IS the device.
// Vulkan:  owns VkDevice + VkPhysicalDevice + queue families.
// ============================================================

class RHI_API IRenderDevice
{
public:
    virtual ~IRenderDevice() = default;

    // ---- Lifecycle ----
    virtual void Init() = 0;
    virtual void Shutdown() = 0;

    // ---- Frame Lifecycle ----
    virtual FrameContext BeginFrame() = 0;
    virtual void EndFrame(FrameContext ctx) = 0;

    // ---- Command Buffer ----
    // Allocate an ICommandBuffer. Valid for the duration of this frame.
    virtual std::unique_ptr<ICommandBuffer> CreateCommandBuffer(
        FrameContext ctx,
        CommandBufferLevel level = CommandBufferLevel::Primary) = 0;

    // ---- Resource Creation ----
    virtual BufferHandle   CreateBuffer(const BufferDesc& desc, const void* initialData = nullptr) = 0;
    virtual TextureHandle  CreateTexture(const TextureDesc& desc, const void* initialData = nullptr) = 0;
    virtual ShaderHandle   CreateShader(ShaderStage stage, const ShaderBytecode& bytecode) = 0;
    virtual PipelineHandle CreatePipeline(const PipelineDesc& desc) = 0;
    virtual GeometryHandle CreateGeometry(const GeometryDesc& desc) = 0;
    virtual SamplerHandle  CreateSampler(const SamplerDesc& desc) = 0;
    virtual RenderPassHandle   CreateRenderPass(const RenderPassDesc& desc) = 0;
    virtual FramebufferHandle  CreateFramebuffer(const FramebufferDesc& desc) = 0;
    virtual QueryPoolHandle   CreateQueryPool(const QueryPoolDesc& desc) = 0;

    // ---- Resource Query (returns interface for inspecting created resources) ----
    virtual IBuffer*   GetBuffer(BufferHandle handle) = 0;
    virtual ITexture*  GetTexture(TextureHandle handle) = 0;
    virtual IShader*   GetShader(ShaderHandle handle) = 0;
    virtual IPipeline* GetPipeline(PipelineHandle handle) = 0;
    virtual IGeometry* GetGeometry(GeometryHandle handle) = 0;
    virtual ISampler*  GetSampler(SamplerHandle handle) = 0;

    // ---- Window Events ----
    virtual void OnResize(uint32_t width, uint32_t height) = 0;
    virtual void WaitIdle() = 0;
    virtual void* GetNativeDevice() = 0;
};

} // namespace rhi
