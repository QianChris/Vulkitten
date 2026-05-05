#include "vktpch.h"
#include "Entity.h"

namespace Vulkitten {

    Entity::Entity(entt::entity handle, Scene* scene)
        : m_EntityHandle(handle), m_Scene(scene)
    {
    }

}