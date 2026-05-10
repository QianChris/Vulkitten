#pragma once

#include <Vulkitten.h>

#include <glm/glm.hpp>
#include <entt/entt.hpp>

#include "Vulkitten/Scene/Entity.h"
#include "Vulkitten/Scene/SceneCamera.h"
#include "Panel/SceneHierarchyPanel.h"
#include "Panel/PropertyPanel.h"
#include "Panel/PerformancePanel.h"
#include "Panel/ViewportPanel.h"
#include "Panel/ResourcePanel.h"

class DefaultLayer : public Vulkitten::Layer
{
public:
    DefaultLayer();
    virtual ~DefaultLayer() = default;

    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnUpdate(Vulkitten::Timestep timestep) override;
    virtual void OnImguiRender() override;
    virtual void OnEvent(Vulkitten::Event& event) override;

private:
    void CreateTestScene();

    void NewScene();
    void OpenScene();
    void SaveSceneAs();

    bool OnKeyPressed(Vulkitten::KeyPressedEvent& event);

    Vulkitten::Ref<Vulkitten::Texture2D> m_Texture;
    Vulkitten::Ref<Vulkitten::Texture2D> m_LogoTexture;

    Vulkitten::Ref<Vulkitten::Scene> m_Scene;

    std::vector<std::pair<std::string, float>> m_ProfileResults;

Vulkitten::SceneHierarchyPanel m_SceneHierarchyPanel;
    Vulkitten::PropertyPanel m_PropertyPanel;
    Vulkitten::PerformancePanel m_PerformancePanel;
    Vulkitten::ViewportPanel m_ViewportPanel;
    Vulkitten::ResourcePanel m_ResourcePanel;

    std::string m_CurrentScenePath;
};