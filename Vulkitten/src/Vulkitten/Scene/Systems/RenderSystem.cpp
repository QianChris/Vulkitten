#include "vktpch.h"
#include "RenderSystem.h"
#include "Vulkitten/Scene/Scene.h"
#include "Vulkitten/Renderer/Renderer.h"

namespace Vulkitten {

    bool RenderQuadComponent(entt::registry& registry)
    {
        auto view = registry.view<const TransformComponent, SpriteRendererComponent>();
        for (auto entity : view)
        {
            auto& [transform, sprite] = view.get<TransformComponent, SpriteRendererComponent>(entity);
            //Renderer2D::DrawQuad(transform.GetTransform(), sprite.Texture, sprite.TilingFactor, sprite.Color);

            auto graph = Renderer::GetRenderGraph();
            graph->AddCommand(DrawQuadCommand{
                sprite.Color,
                sprite.Texture,
                sprite.TilingFactor,
                transform.GetTransform()
                });
        }

        return true;
    }

    bool RenderSystem::OnUpdate(Scene& scene, Timestep timestep, bool shouldRender)
    {
        auto& registry = scene.GetRegistry();
        bool ret = false;

        // Add a clear command at the start of each frame
        auto graph = Renderer::GetRenderGraph();
        graph->AddCommand(ClearCommand{
            glm::vec4(0.1f, 0.1f, 0.1f, 1.0f),  // color
            1.0f,                                   // depth
            0,                                      // stencil
            true,                                   // clearColor
            true,                                   // clearDepth
            false                                   // clearStencil
        });

        ret = RenderQuadComponent(registry) || ret;

        return ret || shouldRender;
    }

}