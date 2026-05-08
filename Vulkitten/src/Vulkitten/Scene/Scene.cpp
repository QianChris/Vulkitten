#include "vktpch.h"
#include "Scene.h"
#include "ScriptableEntity.h"

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

    void Scene::DestroyEntity(Entity entity)
    {
        if (entity)
        {
            m_Registry.destroy(entity.m_EntityHandle);
        }
    }

    void Scene::SetCameraAspectRatio(float aspectRatio)
    {
        auto cameraView = m_Registry.view<CameraComponent>();
        for (auto entity : cameraView)
        {
            auto& cameraComponent = cameraView.get<CameraComponent>(entity);
            if (!cameraComponent.FixedAspectRatio)
            {
                cameraComponent.Camera.SetAspectRatio(aspectRatio);
            }
        }
    }

    void Scene::OnUpdate(Timestep ts)
    {
        TickScripts(ts);

        Entity cameraEntity(entt::null, this);
        auto cameraView = m_Registry.view<CameraComponent>();
        for (auto entity : cameraView)
        {
            if (cameraView.get<CameraComponent>(entity).Primary)
            {
                cameraEntity = Entity(entity, this);
                break;
            }
        }

        if (cameraEntity)
        {
            auto& cameraComponent = cameraEntity.GetComponent<CameraComponent>();
            auto& transform = cameraEntity.GetComponent<TransformComponent>();
            glm::mat4 viewMatrix = glm::inverse(transform.GetTransform());
            cameraComponent.Camera.SetViewMatrix(viewMatrix);
            RenderScene(cameraComponent.Camera);
        }
    }

    void Scene::TickScripts(Timestep ts)
    {
        auto scriptView = m_Registry.view<NativeScriptComponent>();
        for (auto entity : scriptView)
        {
            auto& scriptComponent = scriptView.get<NativeScriptComponent>(entity);
            
            if (scriptComponent.Instance == nullptr)
            {
                if (scriptComponent.InstantiateScript)
                {
                    scriptComponent.Instance = scriptComponent.InstantiateScript();
                    scriptComponent.Instance->m_Entity = Entity(entity, this);
                    scriptComponent.Instance->OnCreate();
                }
            }

            if (scriptComponent.Instance)
            {
                scriptComponent.Instance->m_Entity = Entity(entity, this);
                scriptComponent.Instance->OnUpdate(ts);
            }
        }
    }

    void Scene::RenderScene(Camera& camera)
    {
        Renderer2D::BeginScene(camera);
        auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);

        for (auto entity : group) {
            auto& [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);

            if (sprite.Texture) {
                Renderer2D::DrawQuad(transform.GetTransform(), sprite.Texture, sprite.TilingFactor, sprite.Color);
            } else {
                Renderer2D::DrawQuad(transform.GetTransform(), sprite.Color);
            }
        }

        Renderer2D::EndScene();
    }

    void Scene::OnEvent(Event& event)
    {
    }
     
}