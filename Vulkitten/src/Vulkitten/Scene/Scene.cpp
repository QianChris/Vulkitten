#include "vktpch.h"
#include "Scene.h"
#include "ScriptableEntity.h"
#include "Entity.h"
#include "Vulkitten/Renderer/Renderer.h"

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
        
        return Entity(static_cast<entt::entity>(entity), this);
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

    Entity Scene::GetPrimaryCameraEntity()
    {
        Entity cameraEntity(entt::null, this);
        auto cameraView = m_Registry.view<CameraComponent>();
        for (auto entity : cameraView)
        {
            if (cameraView.get<CameraComponent>(entity).Primary)
            {
                cameraEntity = Entity(entity, this);                break;
            }
        }
        return cameraEntity;
    }

    Entity Scene::GetEntityByID(uint32_t id)
    {
        return Entity{ entt::entity(id), this };
    }

    void Scene::OnUpdate(Timestep ts)
    {
        // Logic tick
        TickScripts(ts);

        bool shouldRender = false;
        for (auto& system : m_Systems)
        {
            if (system->OnUpdate(*this, ts, shouldRender)) {
                shouldRender = true;
            }
		}

        // RenderGraph path: systems have populated the graph with commands.
        // Store camera for passes, skip direct RenderScene.
        if (!m_Systems.empty() && shouldRender)
        {
            Camera* camera = m_EditorCamera;

            if (!camera)
            {
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
                    cameraComponent.Camera.SetTransform(transform.GetTransform());
                    camera = &cameraComponent.Camera;
                }
            }

            if (camera)
            {
                auto* graph = Renderer::GetRenderGraph();
                graph->SetSceneCamera(camera);
                graph->SetViewProjection(camera->GetViewProjectionMatrix());
            }

            return;  // RenderGraph will handle execution in Renderer::Render()
        }

        if (!shouldRender) { return; }

        // Legacy path (no systems registered): direct RenderScene call
        if (m_EditorCamera)
        {
            RenderScene(*m_EditorCamera);
            return;
        }

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
            cameraComponent.Camera.SetTransform(transform.GetTransform());
            RenderScene(cameraComponent.Camera);
        }
    }

    void Scene::OnUpdateRuntime(Timestep ts)
    {
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
        auto gpuEmitterView = m_Registry.view<GpuEmitterComponent>();
        for (auto entity : gpuEmitterView)
        {
            auto emitterInstance = m_EmitterManager.GetOrCreateEmitterInstance(Entity(entity, this));
            if (emitterInstance)
            {
                emitterInstance->Update();
                emitterInstance->Render(camera);
            }
        }

        Renderer2D::BeginScene(camera);

        auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
        for (auto entity : group) {
            auto& [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);
            int entityID = (int)entt::to_integral(entity);

            if (sprite.Texture) {
                Renderer2D::DrawQuad(transform.GetTransform(), sprite.Texture, sprite.TilingFactor, sprite.Color, entityID);
            } else {
                Renderer2D::DrawQuad(transform.GetTransform(), sprite.Color, entityID);
            }
        }

        Renderer2D::EndScene();
    }

    void Scene::OnRuntimeStart()
    {
    }

    void Scene::OnRuntimeStop()
    {
    }

    void Scene::OnEvent(Event& event)
    {
    }
     
}