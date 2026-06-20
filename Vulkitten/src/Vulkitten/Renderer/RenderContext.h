#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Renderer/Renderer.h"
#include "Vulkitten/Renderer/RenderUtils.h"
#include "Vulkitten/Renderer/Shader.h"

namespace Vulkitten {

// ============================================================
// RenderContext — rendering subsystem singleton.
//
// Created via ClassFactory with a specified backend (OpenGL/Vulkan).
// Owns the Renderer instance, RenderGraph, and RenderUtils.
// Replaces the old static Renderer::Init/Render/Shutdown pattern.
//
// Usage:
//   RenderContext::Get().Init();
//   RenderContext::Get().Execute();          // per-frame
//   RenderContext::Get().GetRenderGraph();   // graph access
// ============================================================

class VKT_API RenderContext
{
public:
    RenderContext(Device* device, GpuResourceManager& resources, ShaderManager& shaders);
    ~RenderContext() = default;

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
    static RenderContext& Get() { return *s_Instance; }

private:
    static RenderContext* s_Instance;
    Renderer     m_Renderer;
    RenderUtils  m_RenderUtils;
    ShaderLibrary m_ShaderLibrary;
};

} // namespace Vulkitten
