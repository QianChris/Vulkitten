#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Renderer/IRenderer.h"
#include "Vulkitten/Renderer/RendererAPI.h"
#include "Vulkitten/Renderer/RenderGraph/RenderGraph.h"
#include "Vulkitten/Renderer/FrameContext.h"
#include "Vulkitten/Renderer/Shader.h"

namespace Vulkitten {

class IDevice;
class IGpuResourceManager;

// ============================================================
// Renderer — OpenGL implementation of IRenderer.
//
// All platform-specific types (OpenGLDevice, GpuResourceManager)
// are hidden in the .cpp file. The header only exposes IRenderer
// interface types (IDevice, IGpuResourceManager).
// ============================================================
class VKT_API Renderer : public IRenderer
{
public:
    explicit Renderer(const RendererConfig& config);
    ~Renderer();

    // ---- IRenderer Lifecycle ----
    void Init() override;
    void Shutdown() override;

    // ---- IRenderer Per-Frame ----
    void BeginFrame() override;
    void Execute() override;
    void EndFrame() override;

    // ---- IRenderer Subsystem Access ----
    IDevice&              GetDevice() override;
    IGpuResourceManager&  GetResourceManager() override;
    RenderGraph*          GetRenderGraph() override     { return m_RenderGraph; }
    ShaderLibrary&        GetShaderLibrary() override   { return m_ShaderLibrary; }

    // ---- IRenderer Window Events ----
    void OnWindowResize(uint32_t width, uint32_t height) override;

    // ---- Internal (used by Passes via static_cast<Renderer&>) ----
    RendererAPI* GetRendererAPI() { return m_RendererAPI; }

    inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

private:
    const RendererConfig& m_Config;

    Scope<IDevice>             m_Device;
    Scope<IGpuResourceManager> m_Resources;
    RenderGraph*               m_RenderGraph = nullptr;
    RendererAPI*               m_RendererAPI = nullptr;
    ShaderLibrary              m_ShaderLibrary;
    Scope<FrameContext>        m_FrameContext;
};

} // namespace Vulkitten
