#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Core/Timestep.h"
#include "Vulkitten/Events/Event.h"
#include "Vulkitten/Renderer/Renderer2D.h"
#include "Vulkitten/Renderer/Camera.h"

#include "Components.h"
#include "Entity.h"

#include "entt/entt.hpp"

namespace Vulkitten {

    class VKT_API Scene
    {
    public:
        Scene();
        ~Scene();

        void OnUpdate(Timestep ts);
        void OnEvent(Event& event);

        Entity CreateEntity(std::string name = "UnnamedEntity");
        void DestroyEntity(Entity entity);
        void SetCameraAspectRatio(float aspectRatio);
        Entity GetPrimaryCameraEntity();

        void SetEditorCamera(Camera* camera) { m_EditorCamera = camera; }
        Camera* GetEditorCamera() { return m_EditorCamera; }

        entt::registry& GetRegistry() { return m_Registry; }

    private:
        void TickScripts(Timestep ts);
        void RenderScene(Camera& camera);

    private:
        entt::registry m_Registry;
        Camera* m_EditorCamera = nullptr;

        friend class Entity;
        friend class SceneSerializer;
    };

}