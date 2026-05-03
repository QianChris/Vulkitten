#include "vktpch.h"
#include "Scene.h"

namespace Vulkitten {

    Scene::Scene()
    {
    }

    Scene::~Scene()
    {
    }

    void Scene::OnUpdate(float deltaTime)
    {
        auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);

        // Reverse order of creating entity
        for (auto entity : group) {
            auto& [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);

            if (sprite.Texture) {
                Renderer2D::DrawQuad(transform.Transform, sprite.Texture, sprite.TilingFactor, sprite.Color);
            } else {
                Renderer2D::DrawQuad(transform.Transform, sprite.Color);
            }
        }
    }

    void Scene::OnEvent(Event& event)
    {
    }
     
}