#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Renderer/Renderer.h"
#include "Vulkitten/Renderer/RenderUtils.h"
#include "Vulkitten/Renderer/Shader.h"

namespace Vulkitten {

// ============================================================
// RendererSubsystem — rendering subsystem singleton.
//
// Created by Application. Owns the Renderer instance, RenderGraph,
// and RenderUtils. Replaces the old static Renderer pattern.
//
// Usage:
//   RendererSubsystem::Get().Init();
//   RendererSubsystem::Get().Execute();          // per-frame
//   RendererSubsystem::Get().GetRenderGraph();   // graph access
// ============================================================

class VKT_API RendererSubsystem
{
public:
    RendererSubsystem(IDevice* device, IGpuResourceManager& resources, ShaderManager& shaders);
    ~RendererSubsystem() = default;

    // ---- Lifecycle ----

    void Init();
    void Shutdown();

    // ---- Per-Frame Execution ----

    void Execute();

    // ---- Subsystem Access ----

    Renderer&            GetRenderer()     { return m_Renderer; }
    RenderGraph*         GetRenderGraph()  { return m_Renderer.GetRenderGraph(); }
    RenderUtils&         GetRenderUtils()  { return m_RenderUtils; }

    // ---- Window Events ----

    void OnWindowResize(uint32_t width, uint32_t height);

    // ---- ShaderLibrary Access ----
    ShaderLibrary& GetShaderLibrary() { return m_ShaderLibrary; }

    // ---- Global accessor ----
    static RendererSubsystem& Get() { return *s_Instance; }

private:
    static RendererSubsystem* s_Instance;
    Renderer     m_Renderer;
    RenderUtils  m_RenderUtils;
    ShaderLibrary m_ShaderLibrary;
};

} // namespace Vulkitten
