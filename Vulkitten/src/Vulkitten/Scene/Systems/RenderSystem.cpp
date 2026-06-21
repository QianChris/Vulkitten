#include "vktpch.h"
#include "RenderSystem.h"
#include "Vulkitten/Scene/Scene.h"
#include "Vulkitten/Scene/SceneContext.h"
#include "Vulkitten/Renderer/RenderGraph/RenderGraph.h"
#include "Vulkitten/Renderer/RenderGraph/RenderCommand.h"

namespace Vulkitten {
const std::string RenderSystem::s_Name = "RenderSystem";
}

namespace Vulkitten {

    static bool RenderQuadComponent(entt::registry& registry, RenderGraph* graph)
    {
        auto view = registry.view<const TransformComponent, SpriteRendererComponent>();
        for (auto entity : view)
        {
            auto& [transform, sprite] = view.get<TransformComponent, SpriteRendererComponent>(entity);

            graph->AddCommand(DrawQuadCommand{
                sprite.Color,
                sprite.Texture,
                sprite.TilingFactor,
                transform.GetTransform()
                });
        }

        return true;
    }

    bool RenderSystem::OnUpdate(Scene& scene, Timestep timestep, bool shouldRender, SceneContext& ctx)
    {
        auto& registry = scene.GetRegistry();
        bool ret = false;

        auto* graph = &ctx.GetRenderGraph();

        // Add a clear command at the start of each frame
        graph->AddCommand(ClearCommand{
            glm::vec4(0.1f, 0.1f, 0.1f, 1.0f),  // color
            1.0f,                                   // depth
            0,                                      // stencil
            true,                                   // clearColor
            true,                                   // clearDepth
            false                                   // clearStencil
        });

        ret = RenderQuadComponent(registry, graph) || ret;

        return ret || shouldRender;
    }

}