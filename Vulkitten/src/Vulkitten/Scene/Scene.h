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

        void Step() {}
        void OnUpdate(Timestep ts);
        void OnEvent(Event& event);

        Entity CreateEntity(std::string name = "UnnamedEntity");
        void DestroyEntity(Entity entity);
        void SetCameraAspectRatio(float aspectRatio);
        Entity GetPrimaryCameraEntity();
        Entity GetEntityByID(uint32_t id);

		bool IsRunning() const { return m_IsRunning; }
		bool IsPaused() const { return m_IsPaused; }
		void SetPaused(bool paused) { m_IsPaused = paused; }

        void SetEditorCamera(Camera* camera) { m_EditorCamera = camera; }
        Camera* GetEditorCamera() { return m_EditorCamera; }

        entt::registry& GetRegistry() { return m_Registry; }

    private:
        void TickScripts(Timestep ts);
        void RenderScene(Camera& camera);

    private:
        entt::registry m_Registry;
        Camera* m_EditorCamera = nullptr;
        
		bool m_IsRunning = false;
		bool m_IsPaused = false;
		int m_StepFrames = 0;

        friend class Entity;
        friend class SceneSerializer;
    };

}