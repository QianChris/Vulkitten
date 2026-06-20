#include "vktpch.h"
#include "Scene.h"
#include "ScriptableEntity.h"
#include "Entity.h"
#include "Vulkitten/Renderer/RenderContext.h"
#include "Vulkitten/Renderer/RenderGraph/RenderGraph.h"

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

        if (m_SystemOrder.empty())
        {
            for (auto& system : m_Systems)
            {
                if (system->OnUpdate(*this, ts, shouldRender))
                    shouldRender = true;
            }
        }
        else
        {
            std::vector<System*> ordered, unordered;
            for (auto& system : m_Systems)
                unordered.push_back(system.get());

            for (const auto& name : m_SystemOrder)
            {
                for (auto it = unordered.begin(); it != unordered.end(); ++it)
                {
                    if ((*it)->GetName() == name)
                    {
                        ordered.push_back(*it);
                        unordered.erase(it);
                        break;
                    }
                }
            }

            for (auto* system : ordered)
                if (system->OnUpdate(*this, ts, shouldRender))
                    shouldRender = true;
            for (auto* system : unordered)
                if (system->OnUpdate(*this, ts, shouldRender))
                    shouldRender = true;
        }

        if (!shouldRender) { return; }

        // RenderGraph path: populate per-frame scene data for passes
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
            PerFrameSceneData perFrameData;
            perFrameData.Camera = camera;
            perFrameData.ViewProjection = camera->GetViewProjectionMatrix();
            perFrameData.Scene = this;
            RenderContext::Get().GetRenderGraph()->SetPerFrameData(perFrameData);
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