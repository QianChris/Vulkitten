#pragma once

#include "vktpch.h"
#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Events/Event.h"
#include "Vulkitten/Core/Timestep.h"

namespace Vulkitten
{
    class VKT_API Layer
    {
    public:
        Layer(const std::string& name = "Layer");
        virtual ~Layer();

        virtual void OnAttach() {}
        virtual void OnDetach() {}
        virtual void OnUpdate(Timestep timestep) {}
        virtual void OnImguiRender() {}
        virtual void OnEvent(Event& event) {}

        inline constexpr const std::string& GetName() const { return m_DebugName; }
    protected:
        const std::string m_DebugName;
    };
}