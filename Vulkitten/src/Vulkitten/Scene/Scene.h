#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Events/Event.h"
#include "Vulkitten/Renderer/Renderer2D.h"
#include "Vulkitten/Scene/Components.h"
#include "entt/entt.hpp"

namespace Vulkitten {

    class VKT_API Scene
    {
    public:
        Scene();
        ~Scene();

        void OnUpdate(float deltaTime);
        void OnEvent(Event& event);

        entt::entity CreateEntity() { return m_Registry.create(); }

        entt::registry& GetRegistry() { return m_Registry; }

    private:
        entt::registry m_Registry;
    };

}