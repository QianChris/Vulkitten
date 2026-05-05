#include "vktpch.h"
#include "Scene.h"

namespace Vulkitten {

    Scene::Scene()
    {
    }

    Scene::~Scene()
    {
    }

    Entity Scene::CreateEntity(std::string name)
    {
        auto entity = m_Registry.create();
        m_Registry.emplace<TransformComponent>(entity);
        m_Registry.emplace<TagComponent>(entity, name);
        return Entity(entity, this);
    }

    void Scene::OnUpdate(Timestep ts)
    {
        TickScripts(ts);
        RenderScene();
    }

    void Scene::TickScripts(Timestep ts)
    {

    }

    void Scene::RenderScene()
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