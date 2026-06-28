#pragma once

#include "Vulkitten/Core/Core.h"

namespace Vulkitten {

class IRenderer;
class RenderGraph;

// ============================================================
// SceneContext - per-frame context injected into Scene::OnUpdate.
//
// Provides Scene and its Systems with access to the rendering
// backend without relying on global singletons. Created by the
// application layer each frame and passed down to all Layers
// and their Scenes.
//
// Future: can be extended with IInput, IAudio, etc.
// ============================================================
class SceneContext
{
public:
    SceneContext(IRenderer& renderer, RenderGraph& graph)
        : m_Renderer(renderer)
        , m_RenderGraph(graph)
    {
    }

    IRenderer&  GetRenderer()    { return m_Renderer; }
    RenderGraph& GetRenderGraph() { return m_RenderGraph; }

private:
    IRenderer&  m_Renderer;
    RenderGraph& m_RenderGraph;
};

} // namespace Vulkitten
