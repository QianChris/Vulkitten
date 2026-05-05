#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Scene/Scene.h"
#include "Vulkitten/Scene/Entity.h"

namespace Vulkitten {

    class SceneHierarchyPanel
    {
    public:
        SceneHierarchyPanel() = default;
        SceneHierarchyPanel(const Ref<Scene>& scene);

        void SetContext(const Ref<Scene>& scene);
        void OnImGuiRender();

    private:
        void DrawSceneHierarchy();
        void DrawEntityView();
        void DrawComponentView();
        void DrawEntityComponents(Entity entity);
        void DrawComponentEntities(const char* componentName, std::vector<std::pair<uint32_t, Entity>>& entities);

    private:
        Ref<Scene> m_Context;

        int m_SelectedEntityView = 0;
        uint32_t m_SelectedEntityID = 0;
        int m_SelectedComponentIndex = -1;
        std::string m_SelectedComponentType;
    };

}