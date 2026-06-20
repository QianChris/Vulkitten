#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Renderer/IRenderer.h"
#include "Vulkitten/Renderer/RendererAPI.h"
#include "Vulkitten/Renderer/RenderGraph/RenderGraph.h"
#include "Vulkitten/Renderer/FrameContext.h"
#include "Vulkitten/Renderer/GpuResourceManager.h"

namespace Vulkitten {

class IDevice;
class ShaderManager;

// ============================================================
// Renderer — scene-level renderer instance.
//
// Implements IRenderer, the sole backend interface exposed to
// platform and scene layers. Owned by RendererSubsystem.
//
// Lifecycle:
//   Init → BeginFrame → Execute → EndFrame → ... → Shutdown
// ============================================================

class VKT_API Renderer : public IRenderer
{
public:
    Renderer(IDevice* device, GpuResourceManager& resources, ShaderManager& shaders);
    ~Renderer();

    // ---- IRenderer Lifecycle ----

    void Init() override;
    void Shutdown() override;

    // ---- IRenderer Per-Frame ----

    void BeginFrame() override;
    void Execute() override;
    void EndFrame() override;

    // ---- IRenderer Subsystem Access ----

    IDevice&              GetDevice() override          { return *m_Device; }
    IGpuResourceManager&  GetResourceManager() override { return m_Resources; }
    ShaderManager&        GetShaderManager()            { return m_Shaders; }
    RenderGraph*          GetRenderGraph() override     { return m_RenderGraph; }
    RendererAPI*          GetRendererAPI()              { return m_RendererAPI; }

    // ---- IRenderer Window Events ----

    void OnWindowResize(uint32_t width, uint32_t height) override;

    inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

private:
    IDevice*             m_Device;
    GpuResourceManager&  m_Resources;
    ShaderManager&       m_Shaders;
    RenderGraph*         m_RenderGraph = nullptr;
    RendererAPI*         m_RendererAPI = nullptr;

    // Per-frame context (created each BeginFrame, consumed by Execute/EndFrame)
    Scope<FrameContext>   m_FrameContext;
};

} // namespace Vulkitten
