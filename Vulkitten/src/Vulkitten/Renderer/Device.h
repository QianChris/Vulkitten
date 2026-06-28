#pragma once

#include "Vulkitten/Core/Core.h"

#include "Vulkitten/RHI/Handle.hpp"
#include "Vulkitten/RHI/RHIResourceDescs.hpp"
#include "Vulkitten/RHI/RHIPipelineDesc.hpp"

#include <memory>
#include <cstdint>

namespace Vulkitten {

// Forward declarations
struct FrameContext;
namespace rhi { class ICommandBuffer; }

// ============================================================
// ShaderBytecode - shader binary data + entry point
// ============================================================
// Vulkan: SPIR-V binary (detected by magic number 0x07230203)
// OpenGL: GLSL source or SPIR-V binary
struct ShaderBytecode
{
    const uint8_t* Data = nullptr;
    size_t         Size = 0;
    const char*    EntryPoint = "main";
};

// ============================================================
// IDevice - abstract GPU device interface.
//
// Represents a logical GPU device. Owns the frame lifecycle
// (swapchain acquire → command recording → submit → present),
// creates all GPU resources, and provides command buffers.
//
// For OpenGL: the GL context IS the device.
// For Vulkan: owns VkDevice + VkPhysicalDevice + queue families.
// ============================================================
class VKT_API IDevice
{
public:
    virtual ~IDevice() = default;

    // ---- Lifecycle ----
    virtual void Init() = 0;
    virtual void Shutdown() = 0;

    // ---- Frame Lifecycle (IDevice owns beginFrame/endFrame) ----
    // beginFrame: wait for previous frame's fence, acquire swapchain image,
    //             begin command buffer, return FrameContext.
    // endFrame:   end command buffer, submit to queue, present swapchain image.
    virtual FrameContext beginFrame() = 0;
    virtual void endFrame(FrameContext ctx) = 0;

    // ---- Command Buffer ----
    // Allocate an ICommandBuffer from the FrameContext's command pool.
    // The returned buffer is valid for the duration of this frame.
    virtual rhi::ICommandBuffer* createCommandBuffer(FrameContext ctx) = 0;

    // ---- Resource Creation (all return rhi::Handle<>) ----
    virtual rhi::BufferHandle   createBuffer(const rhi::BufferDesc& desc, const void* initialData = nullptr) = 0;
    virtual rhi::TextureHandle  createTexture(const rhi::TextureDesc& desc, const void* initialData = nullptr) = 0;
    virtual rhi::ShaderHandle   createShader(rhi::ShaderStage stage, const ShaderBytecode& bytecode) = 0;
    virtual rhi::PipelineHandle createPipeline(const rhi::PipelineDesc& desc) = 0;
    virtual rhi::GeometryHandle createGeometry(const rhi::GeometryDesc& desc) = 0;
    virtual rhi::SamplerHandle  createSampler(const rhi::SamplerDesc& desc) = 0;
    virtual rhi::RenderPassHandle   createRenderPass(const rhi::RenderPassDesc& desc) = 0;
    virtual rhi::FramebufferHandle  createFramebuffer(const rhi::FramebufferDesc& desc) = 0;

    // ---- Window Events ----
    virtual void onResize(uint32_t width, uint32_t height) = 0;

    // ---- Utilities ----
    virtual void waitIdle() = 0;             // Force GPU to finish all work
    virtual void* getNativeDevice() const = 0;     // VK: VkDevice; GL: nullptr

    // ---- Legacy (will be removed after Tasks 14/18 transition) ----
    virtual void Submit(FrameContext& frameContext) = 0;

    // ---- Global Accessor ----
    static IDevice& Get();
};

} // namespace Vulkitten
