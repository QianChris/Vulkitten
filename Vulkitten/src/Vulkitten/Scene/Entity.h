#pragma once

#include "Vulkitten/Core/Core.h"

#include "entt/entt.hpp"

namespace Vulkitten {

    class Scene;

    class VKT_API Entity
    {
    public:
        Entity() = default;
        Entity(entt::entity handle, Scene* scene);
        Entity(const Entity& other) = default;

        bool operator==(const Entity& other) const { return m_EntityHandle == other.m_EntityHandle; }
        bool operator!=(const Entity& other) const { return !(*this == other); }
operator bool() const { return m_EntityHandle != entt::null; }

        uint32_t GetEntityID() const { return static_cast<uint32_t>(m_EntityHandle); }

        template<typename T, typename... Args>
        T& AddComponent(Args&&... args)
        {
            VKT_CORE_ASSERT(!HasComponent<T>(), "Entity already has component!");
            return m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
        }

        template<typename T>
        T& GetComponent()
        {
            VKT_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
            return m_Scene->m_Registry.get<T>(m_EntityHandle);
        }

template<typename T>
        bool HasComponent()
        {
            return m_Scene->GetRegistry().any_of<T>(m_EntityHandle);
        }

        template<typename T>
        void RemoveComponent()
        {
            VKT_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
            m_Scene->m_Registry.remove<T>(m_EntityHandle);
        }

    private:
        entt::entity m_EntityHandle{ entt::null };
        Scene* m_Scene = nullptr;

        friend class Scene;
    };

}