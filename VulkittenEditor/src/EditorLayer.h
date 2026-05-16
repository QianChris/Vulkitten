#pragma once
#include <Vulkitten.h>
#include <glm/glm.hpp>
#include <entt/entt.hpp>

#include "EditorContext.h"
#include "EditorCommand.h"
#include "IPanel.h"
#include "Panel/ViewportPanel.h"
#include "Panel/SceneHierarchyPanel.h"
#include "Panel/PropertyPanel.h"
#include "Panel/PerformancePanel.h"
#include "Panel/ResourcePanel.h"

class EditorLayer : public Vulkitten::Layer {
public:
    EditorLayer();
    virtual ~EditorLayer() = default;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(Vulkitten::Timestep timestep) override;
    void OnImguiRender() override;
    void OnEvent(Vulkitten::Event& event) override;

private:
    void CreateTestScene();
    void NewScene();
    void OpenScene();
    void SaveSceneAs();
    bool OnKeyPressed(Vulkitten::KeyPressedEvent& event);
    bool OnMouseButtonPressed(Vulkitten::MouseButtonPressedEvent& event);
    bool IsViewportFocused() const;

private:
    Vulkitten::Ref<Vulkitten::Texture2D> m_Texture;
    Vulkitten::Ref<Vulkitten::Texture2D> m_LogoTexture;
    Vulkitten::Ref<Vulkitten::Scene> m_Scene;
    Vulkitten::EditorCamera m_EditorCamera;

    Vulkitten::EditorContext m_Context;
    Vulkitten::CommandSystem m_CommandSystem;

    Vulkitten::Scope<Vulkitten::ViewportPanel> m_ViewportPanel;
    Vulkitten::Scope<Vulkitten::SceneHierarchyPanel> m_SceneHierarchyPanel;
    Vulkitten::Scope<Vulkitten::PropertyPanel> m_PropertyPanel;
    Vulkitten::Scope<Vulkitten::PerformancePanel> m_PerformancePanel;
    Vulkitten::Scope<Vulkitten::ResourcePanel> m_ResourcePanel;

    std::string m_CurrentScenePath;
};