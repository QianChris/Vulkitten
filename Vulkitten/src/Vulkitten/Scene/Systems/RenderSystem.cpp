#include "vktpch.h"
#include "RenderSystem.h"
#include "Vulkitten/Scene/Scene.h"

namespace Vulkitten {

    bool RenderQuadComponent(entt::registry& registry)
    {
        auto view = registry.view<const TransformComponent, SpriteRendererComponent>();
        for (auto entity : view)
        {
            auto& [transform, sprite] = view.get<TransformComponent, SpriteRendererComponent>(entity);
            Renderer2D::DrawQuad(transform.GetTransform(), sprite.Texture, sprite.TilingFactor, sprite.Color);
        }

        return true;
    }

    bool RenderSystem::OnUpdate(Scene& scene, Timestep timestep, bool shouldRender)
    {
        auto& registry = scene.GetRegistry();
        bool ret = false;

        ret = RenderQuadComponent(registry) || ret;
        
        return ret || shouldRender;
    }

}