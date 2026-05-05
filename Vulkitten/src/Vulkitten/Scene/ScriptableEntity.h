#pragma once

#include "Entity.h"
#include "Vulkitten/Core/Timestep.h"

namespace Vulkitten {

    class VKT_API ScriptableEntity
    {
    public:
        virtual ~ScriptableEntity() = default;

        template<typename T>
        T& GetComponent()
        {
            return m_Entity.GetComponent<T>();
        }

        template<typename T>
        void BindComponent()
        {
            m_Entity.AddComponent<T>();
        }

	protected:
		virtual void OnCreate() {}
		virtual void OnDestroy() {}
		virtual void OnUpdate(Timestep ts) {}

protected:
        Entity m_Entity;
		friend class Scene;
		friend struct ScriptComponent;
    };

}