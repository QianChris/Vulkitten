#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Renderer/Renderer.h"
#include "Vulkitten/Renderer/RenderUtils.h"
#include "Vulkitten/Renderer/Shader.h"

namespace Vulkitten {

// ============================================================
// RendererSubsystem — legacy rendering subsystem singleton.
//
// Deprecated: Application now uses IRenderer directly.
// Kept for backward compatibility with existing code.
// ============================================================

class VKT_API RendererSubsystem
{
public:
    explicit RendererSubsystem(const RendererConfig& config);
    ~RendererSubsystem() = default;

    void Init();
    void Shutdown();
    void Execute();

    Renderer&            GetRenderer()     { return m_Renderer; }
    RenderGraph*         GetRenderGraph()  { return m_Renderer.GetRenderGraph(); }
    RenderUtils&         GetRenderUtils()  { return m_RenderUtils; }

    void OnWindowResize(uint32_t width, uint32_t height);

    ShaderLibrary& GetShaderLibrary() { return m_Renderer.GetShaderLibrary(); }

    static RendererSubsystem& Get() { return *s_Instance; }

private:
    static RendererSubsystem* s_Instance;
    Renderer     m_Renderer;
    RenderUtils  m_RenderUtils;
};

} // namespace Vulkitten
