#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Scene/Scene.h"
#include "Vulkitten/Scene/Entity.h"

#include <functional>
#include <vector>
#include <typeindex>
#include <memory>

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
        void DrawComponentHeader(const char* name, const char* id, std::function<void()> onDelete);
        void DrawAddComponentButton(Entity entity);
        void ProcessDeferredActions();

    private:
        Ref<Scene> m_Context;
        Entity m_SelectedEntity;
        std::vector<std::pair<Entity, std::type_index>> m_ComponentsToRemove;
    };

}