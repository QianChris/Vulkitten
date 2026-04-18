#pragma once

#include "Vulkitten/Core.h"
#include "Vulkitten/Events/Event.h"
#include "Vulkitten/Layer.h"

namespace Vulkitten
{
    class VKT_API ImGuiLayer : public Layer
    {
    public:
        ImGuiLayer();
        ~ImGuiLayer();

        virtual void OnAttach() override;
        virtual void OnDetach() override;
        virtual void OnImguiRender() override;

        void Begin();
        void End();

    private:
        float m_Time = 0.0f;
    };
}