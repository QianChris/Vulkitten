#pragma once

#include "rhi/Core/Types.hpp"
#include "rhi/Core/Handle.hpp"
#include "rhi/ISurface.hpp"
#include "rhi/FrameContext.hpp"

#include <memory>
#include <cstdint>

namespace rhi {

class IRenderDevice;
class ICommandBuffer;
class RenderCommandList;
class ResourceManager;

// ============================================================
// BackendType
// ============================================================

enum class BackendType
{
    OpenGL,
    Vulkan,
};

// ============================================================
// RendererConfig
// ============================================================

struct RendererConfig
{
    BackendType Backend = BackendType::OpenGL;
    ISurface*   Surface = nullptr;
    bool        EnableValidation = true;
    bool        EnableDebugMarkers = true;
    uint32_t    FramesInFlight = 2;
};

// ============================================================
// Renderer — top-level rendering entry point
//
// Owns the IRenderDevice, ResourceManager, and manages the
// frame lifecycle. Provides access to per-frame command lists.
//
// Usage:
//   auto renderer = Renderer::Create(config);
//   while (running) {
//       renderer->BeginFrame();
//       auto& cmd = renderer->GetCommandList();
//       // ... record commands via cmd ...
//       renderer->EndFrame();
//   }
// ============================================================

class Renderer
{
public:
    // Factory: creates the correct backend based on config.Backend
    static std::unique_ptr<Renderer> Create(const RendererConfig& config);

    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    // ---- Frame Lifecycle ----
    void BeginFrame();
    void EndFrame();
    bool IsFrameInProgress() const;

    // ---- Accessors ----
    IRenderDevice&     GetDevice();
    ResourceManager&   GetResources();
    RenderCommandList& GetCommandList();
    ICommandBuffer&    GetCommandBuffer();
    BackendType        GetBackendType() const;
    uint32_t           GetFrameIndex() const;
    uint64_t           GetFrameCount() const;
    uint32_t           GetSwapchainIndex() const;

    // ---- Default Swapchain Resources (created once, reused each frame) ----
    RenderPassHandle   GetDefaultRenderPass();
    FramebufferHandle  GetDefaultFramebuffer();

    // ---- Window Events ----
    void OnResize(uint32_t width, uint32_t height);
    void WaitIdle();

private:
    Renderer() = default;

    struct Impl;
    std::unique_ptr<Impl> m_Impl;
};

} // namespace rhi
