#include "vktpch.h"
#include "GpuParticle.h"
#include "Vulkitten/Scene/Scene.h"

namespace Vulkitten {

    GpuEmitterInstance* GpuEmitterManager::GetOrCreateEmitterInstance(Entity entity)
    {
        if (!m_Initialized)
        {
            Initialize();
		}

        if (m_EmitterInstances.find(entity) != m_EmitterInstances.end())
        {
            return m_EmitterInstances[entity].get();
        }
        else
        {
			auto& emitterComponent = entity.GetComponent<GpuEmitterComponent>();
			auto& transformComponent = entity.GetComponent<TransformComponent>();

            auto emitterInstance = std::make_unique<GpuEmitterInstance>(
                this, emitterComponent.MaxParticles, transformComponent.Position);

            GpuEmitterInstance* emitterInstancePtr = emitterInstance.get();
            m_EmitterInstances[entity] = std::move(emitterInstance);
			return emitterInstancePtr;
        }
    }

    void GpuEmitterManager::Initialize()
    {
        // Initialization code here
		m_Initialized = true;

        // Load Shader to shader library
    }

    void GpuEmitterInstance::Update()
    {
        if (m_MaxParticles > 0 && !m_Initialized)
        {
            Initialize();
		}
    }

    void GpuEmitterInstance::Render(Camera& camera)
    {
    }

    void GpuEmitterInstance::Initialize()
    {
        // Initialization code here
		m_Initialized = true;
    }

}