#pragma once
// Realization, not for api.

#include "Vulkitten/Core/Core.h"

//#include "Vulkitten/Scene/Entity.h"
#include "Vulkitten/Scene/Components.h"
#include "Vulkitten/Renderer/Camera.h"
#include "Vulkitten/Renderer/Shader.h"


namespace Vulkitten {

    class Entity;
    class GpuEmitterManager;

    class GpuEmitterInstance
    {
    public:
        GpuEmitterInstance() = default;
        GpuEmitterInstance(GpuEmitterManager* manager, uint32_t count, glm::vec3 pos)
            : m_Manager(manager), m_MaxParticles(count), m_Position(pos) {
        }
        ~GpuEmitterInstance() = default;

        void Update();
        void Render(Camera& camera);

    private:
        void Initialize();

    private:
        glm::vec3 m_Position{ 0.0f };

        uint32_t m_MaxParticles{ 1000u };
        uint32_t currIdx{ 0u };
        GpuEmitterManager* m_Manager{ nullptr };
        bool m_Initialized{ false };
    };

    class GpuEmitterManager
    {
    public:
        GpuEmitterManager() = default;
        ~GpuEmitterManager() = default;

        GpuEmitterInstance* GetOrCreateEmitterInstance(Entity entity);

		const ShaderLibrary& GetShaderLibrary() { return m_ShaderLibrary; }

    private:
        void Initialize();

    private:
		std::unordered_map<Entity, std::unique_ptr<GpuEmitterInstance>> m_EmitterInstances;
		ShaderLibrary m_ShaderLibrary;

		bool m_Initialized = false;
    };

} // namespace Vulkitten