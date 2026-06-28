#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Core/Timestep.h"
#include "Vulkitten/Events/Event.h"
#include "Vulkitten/Renderer/Camera.h"
#include "Vulkitten/Scene/GpuParticle/GpuParticle.h"
#include "Vulkitten/Scene/Systems/System.h"

#include "Components.h"
#include "Entity.h"

#include "entt/entt.hpp"

namespace Vulkitten {

    class Entity;
    class System;

    class VKT_API Scene
    {
    public:
        Scene();
        ~Scene();

        Entity CreateEntity(std::string name = "UnnamedEntity");
        void DestroyEntity(Entity entity);
        Entity GetPrimaryCameraEntity();
        Entity GetEntityByID(uint32_t id);

        void OnUpdate(Timestep ts, class SceneContext& ctx);

		// With scripts/graph
        void OnUpdateRuntime(Timestep ts);
		// With physics
        void OnUpdateSimulation(Timestep ts) { OnUpdateRuntime(ts); }
        // Step simulation only
        void Step(int frames = 1) { m_StepFrames = frames; };

        void OnRuntimeStart();
        void OnRuntimeStop();
        void OnSimulationStart() { OnRuntimeStart(); }
        void OnSimulationStop() { OnRuntimeStop(); }

        void OnEvent(Event& event);

        void SetCameraAspectRatio(float aspectRatio);

		bool IsRunning() const { return m_IsRunning; }
		bool IsPaused() const { return m_IsPaused; }
		void SetPaused(bool paused) { m_IsPaused = paused; }

        void SetEditorCamera(Camera* camera) { m_EditorCamera = camera; }
        Camera* GetEditorCamera() { return m_EditorCamera; }

        entt::registry& GetRegistry() { return m_Registry; }
        void AddSystem(Scope<System> system) { m_Systems.push_back(std::move(system)); }
        // Configure the execution order of Systems by name.
        // Systems not listed execute last, in their AddSystem order.
        void SetSystemOrder(std::vector<std::string> systemNames) { m_SystemOrder = std::move(systemNames); }
        GpuEmitterManager& GetEmitterManager() { return m_EmitterManager; }

    private:
        void TickScripts(Timestep ts);

    private:
        entt::registry m_Registry;
        std::vector<Scope<System>> m_Systems;
        std::vector<std::string> m_SystemOrder;

        Camera* m_EditorCamera = nullptr;
        GpuEmitterManager m_EmitterManager {};
        
		bool m_IsRunning = false;
		bool m_IsPaused = false;
		int m_StepFrames = 0;

        friend class Entity;
        friend class SceneSerializer;
    };

}