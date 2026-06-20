#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Renderer/RendererAPI.h"
#include "Vulkitten/Renderer/RenderGraph/RenderGraph.h"

namespace Vulkitten {

class Device;
class GpuResourceManager;
class ShaderManager;

// ============================================================
// Renderer — scene-level renderer instance.
//
// Created and owned by RenderContext. Holds references to the
// GPU device, resource manager, and shader manager.
//
// Replaces the old static Renderer class. All methods are now
// instance methods; callers go through RenderContext::Get().
// ============================================================

class VKT_API Renderer
{
public:
    Renderer(Device* device, GpuResourceManager& resources, ShaderManager& shaders);
    ~Renderer() = default;

    // ---- Lifecycle ----

    void Init();
    void Shutdown();

    // ---- Per-Frame ----

    void Execute();

    // ---- Subsystem Access ----

    Device&              GetDevice()          { return *m_Device; }
    GpuResourceManager&  GetResourceManager() { return m_Resources; }
    ShaderManager&       GetShaderManager()   { return m_Shaders; }
    RenderGraph*         GetRenderGraph()     { return m_RenderGraph; }

    // ---- Helpers ----

    void OnWindowResize(uint32_t width, uint32_t height);

    inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

private:
    Device*             m_Device;
    GpuResourceManager& m_Resources;
    ShaderManager&      m_Shaders;
    RenderGraph*        m_RenderGraph = nullptr;
};

} // namespace Vulkitten
