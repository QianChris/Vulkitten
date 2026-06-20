#pragma once
// Realization, not for api.

#include "Vulkitten/Core/Core.h"

//#include "Vulkitten/Scene/Entity.h"
#include "Vulkitten/Scene/Components.h"
#include "Vulkitten/Renderer/Camera.h"
#include "Vulkitten/Renderer/Shader.h"
#include "Vulkitten/Renderer/RenderContext.h"
#include "Vulkitten/Scene/GpuParticle/ParticleStruct.h"

#include <glm/glm.hpp>

typedef unsigned int GLuint;

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
        ~GpuEmitterInstance();

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

        // OpenGL buffer handles
        GLuint m_ArgSSBO[2] = { 0, 0 };
        GLuint m_ParticleSSBO[2] = { 0, 0 };
        GLuint m_UBO = 0;

        // Render resources
        GLuint m_RenderUBO = 0;
        GLuint m_VAO = 0;

        uint32_t m_FrameIndex = 0;
        float m_TotalTime = 0.0f;
    };

    class GpuEmitterManager
    {
    public:
        GpuEmitterManager() = default;
        ~GpuEmitterManager() = default;

        GpuEmitterInstance* GetOrCreateEmitterInstance(Entity entity);

		const ShaderLibrary& GetShaderLibrary() { return RenderContext::Get().GetShaderLibrary(); }

    private:
        void Initialize();

    private:
		std::unordered_map<Entity, std::unique_ptr<GpuEmitterInstance>> m_EmitterInstances;

		bool m_Initialized = false;
    };

} // namespace Vulkitten