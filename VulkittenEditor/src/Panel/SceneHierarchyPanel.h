#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Scene/Scene.h"
#include "Vulkitten/Scene/Entity.h"

namespace Vulkitten {

    class SceneHierarchyPanel
    {
    public:
        static const uint32_t INVALID_SELECT = ~0u;

        SceneHierarchyPanel() = default;
        SceneHierarchyPanel(const Ref<Scene>& scene);

        void SetContext(const Ref<Scene>& scene);
        Entity GetSelectedEntity() const;
        void OnImGuiRender();

    private:
        void DrawSceneHierarchy();
        void DrawEntityView();
        void DrawComponentView();

    private:
        Ref<Scene> m_Context;
        uint32_t m_SelectedEntityID { INVALID_SELECT };
    };

}