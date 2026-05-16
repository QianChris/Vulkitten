#pragma once
#include "IPanel.h"
#include "Vulkitten/Scene/Entity.h"

namespace Vulkitten {

    class SceneHierarchyPanel : public IPanel {
    public:
        SceneHierarchyPanel() = default;
        void OnAttach(EditorContext* context) override;
        void OnUIRender() override;

    private:
        void DrawSceneHierarchy();
        void DrawEntityView();
        void DrawComponentView();

    private:
        uint32_t m_SelectedEntityID { ~0u };
    };

} // namespace Vulkitten