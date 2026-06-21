#pragma once

#include "Vulkitten/Core/Core.h"

#include <cstdint>

namespace Vulkitten {

class IDevice;
class IGpuResourceManager;
class RenderGraph;
class ShaderLibrary;
class FileSystem;
class IWindow;

// ============================================================
// RendererConfig — configuration for creating an IRenderer.
// ============================================================
struct RendererConfig
{
    FileSystem*   FileSys = nullptr;     // Engine file system (for shader loading)
    IWindow*      Window = nullptr;       // Platform window (for surface creation)
};

// ============================================================
// IRenderer — the single backend renderer interface.
//
// The platform/scene layers interact with the renderer exclusively
// through this interface. Application creates the concrete impl.
//
// Lifecycle:
//   Init() → BeginFrame() → [per-frame: Execute] → EndFrame() → Shutdown()
// ============================================================
class VKT_API IRenderer
{
public:
    virtual ~IRenderer() = default;

    // ---- Lifecycle ----
    virtual void Init() = 0;
    virtual void Shutdown() = 0;

    // ---- Per-Frame ----
    virtual void BeginFrame() = 0;
    virtual void Execute() = 0;
    virtual void EndFrame() = 0;

    // ---- Window Events ----
    virtual void OnWindowResize(uint32_t width, uint32_t height) = 0;

    // ---- Subsystem Access ----
    virtual IDevice&              GetDevice() = 0;
    virtual IGpuResourceManager&  GetResourceManager() = 0;
    virtual RenderGraph*          GetRenderGraph() = 0;
    virtual ShaderLibrary&        GetShaderLibrary() = 0;

    // ---- Global Accessor (set by Application after Init) ----
    static IRenderer& Get() { return *s_Current; }

protected:
    static IRenderer* s_Current;
};

} // namespace Vulkitten
