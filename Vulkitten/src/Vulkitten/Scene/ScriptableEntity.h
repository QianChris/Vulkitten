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

	protected:
		virtual void OnCreate() {}
		virtual void OnDestroy() {}
		virtual void OnUpdate(Timestep ts) {}

    private:
        Entity m_Entity;
		friend class Scene;
    };

}