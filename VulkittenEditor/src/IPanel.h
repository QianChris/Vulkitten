#pragma once
#include "EditorContext.h"
#include "Vulkitten/Core/Timestep.h"
#include "Vulkitten/Events/Event.h"

namespace Vulkitten {
    class IPanel {
    public:
        virtual ~IPanel() = default;
        virtual void OnAttach(EditorContext* context) { m_Context = context; }
        virtual void OnDetach() {}
        virtual void OnUpdate(Timestep ts) {}
        virtual void OnUIRender() = 0;
        virtual void OnEvent(Event& event) {}
        
        bool IsOpen = true;
    protected:
        EditorContext* m_Context = nullptr;
    };
}