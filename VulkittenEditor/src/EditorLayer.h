#pragma once
#include <glm/glm.hpp>
#include <entt/entt.hpp>

#include "Vulkitten/Core/Layer.h"

#include "EditorContext.h"
#include "EditorCommand.h"
#include "IPanel.h"
#include "Panel/ViewportPanel.h"
#include "Panel/SceneHierarchyPanel.h"
#include "Panel/PropertyPanel.h"
#include "Panel/PerformancePanel.h"
#include "Panel/ResourcePanel.h"

namespace Vulkitten {

    class EditorLayer : public Layer {
    public:
        EditorLayer();
        virtual ~EditorLayer() = default;

        void OnAttach() override;
        void OnDetach() override;
        void OnUpdate(Timestep timestep) override;
        void OnImguiRender() override;
        void OnEvent(Event& event) override;

    private:
        void CreateTestScene();
        void NewScene();
        void OpenScene();
        void SaveSceneAs();
        bool OnKeyPressed(KeyPressedEvent& event);
        bool OnMouseButtonPressed(MouseButtonPressedEvent& event);
        bool IsViewportFocused() const;

    private:
        Ref<Texture2D> m_Texture;
        Ref<Texture2D> m_LogoTexture;
        Ref<Scene> m_Scene;
        EditorCamera m_EditorCamera;

        EditorContext m_Context;
        CommandSystem m_CommandSystem;

        Scope<ViewportPanel> m_ViewportPanel;
        Scope<SceneHierarchyPanel> m_SceneHierarchyPanel;
        Scope<PropertyPanel> m_PropertyPanel;
        Scope<PerformancePanel> m_PerformancePanel;
        Scope<ResourcePanel> m_ResourcePanel;

        std::string m_CurrentScenePath;


        enum class SceneState
        {
            Edit = 0, Play = 1, Simulate = 2
        };
        SceneState m_SceneState = SceneState::Edit;
    };

}