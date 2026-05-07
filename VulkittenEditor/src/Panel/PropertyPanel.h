#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Scene/Scene.h"
#include "Vulkitten/Scene/Entity.h"

namespace Vulkitten {

    class PropertyPanel
    {
    public:
        PropertyPanel() = default;
        PropertyPanel(const Ref<Scene>& scene);

        void SetContext(const Ref<Scene>& scene);
        void SetSelectedEntity(Entity entity);
        void OnImGuiRender();

    private:
        void DrawEntityComponents(Entity entity);

    private:
        Ref<Scene> m_Context;
        Entity m_SelectedEntity;
    };

}