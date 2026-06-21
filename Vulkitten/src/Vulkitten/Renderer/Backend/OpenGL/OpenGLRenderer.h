#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Renderer/Renderer.h"
#include "Vulkitten/Renderer/RendererAPI.h"

namespace Vulkitten {

// ============================================================
// OpenGLRenderer — OpenGL implementation of Renderer (IRenderer).
// ============================================================
class VKT_API OpenGLRenderer : public Renderer
{
public:
    explicit OpenGLRenderer(const RendererConfig& config);
    ~OpenGLRenderer();

    void Init() override;
    void Shutdown() override;

    void BeginFrame() override;
    void Execute() override;
    void EndFrame() override;
    void OnWindowResize(uint32_t width, uint32_t height) override;

    RendererAPI* GetRendererAPI() { return m_RendererAPI; }
    inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

private:
    RendererAPI* m_RendererAPI = nullptr;
};

} // namespace Vulkitten
