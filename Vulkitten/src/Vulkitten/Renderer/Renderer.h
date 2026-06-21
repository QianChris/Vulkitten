#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Renderer/IRenderer.h"
#include "Vulkitten/Renderer/IGpuResourceManager.h"
#include "Vulkitten/Renderer/RenderGraph/RenderGraph.h"
#include "Vulkitten/Renderer/FrameContext.h"
#include "Vulkitten/Renderer/Shader.h"

namespace Vulkitten {

// ============================================================
// Renderer — platform-agnostic IRenderer base implementation.
//
// Holds common IRenderer state (IDevice, IGpuResourceManager,
// RenderGraph, ShaderLibrary). Concrete backends (OpenGLRenderer,
// VkRenderer) derive from this and implement Init()/Shutdown().
// ============================================================
class VKT_API Renderer : public IRenderer
{
public:
    explicit Renderer(const RendererConfig& config);
    ~Renderer() override;

    // ---- Subsystem Access (shared impl) ----
    IDevice&              GetDevice() override;
    IGpuResourceManager&  GetResourceManager() override;
    RenderGraph*          GetRenderGraph() override  { return m_RenderGraph; }
    ShaderLibrary&        GetShaderLibrary() override { return m_ShaderLibrary; }

    // ---- Window Events ----
    void OnWindowResize(uint32_t width, uint32_t height) override;

protected:
    const RendererConfig& m_Config;
    Scope<IDevice>             m_Device;
    Scope<IGpuResourceManager> m_Resources;
    RenderGraph*               m_RenderGraph = nullptr;
    ShaderLibrary              m_ShaderLibrary;
    Scope<FrameContext>        m_FrameContext;
};

} // namespace Vulkitten
