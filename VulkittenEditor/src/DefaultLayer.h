#pragma once

#include <Vulkitten.h>

#include <glm/glm.hpp>
#include <entt/entt.hpp>

#include "Vulkitten/Scene/Entity.h"
#include "Vulkitten/Scene/SceneCamera.h"

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
    void ImGuiTest();
    void UpdateViewportFramebuffer(uint32_t width, uint32_t height);
    void DrawSceneHierarchy();

    void DrawEntityView();
    void DrawComponentView();

    void DrawEntityComponents(Vulkitten::Entity entity);
    void DrawComponentEntities(const char* componentName, std::vector<std::pair<uint32_t, Vulkitten::Entity>>& entities);

    template<typename T>
    void DrawComponentData(T& component);

    Vulkitten::Ref<Vulkitten::Texture2D> m_Texture;
    Vulkitten::Ref<Vulkitten::Texture2D> m_LogoTexture;

    Vulkitten::Ref<Vulkitten::FrameBuffer> m_Framebuffer;
    uint32_t m_ViewportWidth = 1280;
    uint32_t m_ViewportHeight = 720;

    Vulkitten::Scene m_Scene;
    std::vector<Vulkitten::Entity> m_Entities;
    Vulkitten::Entity m_CameraEntity;

    std::vector<std::pair<std::string, float>> m_ProfileResults;

    int m_SelectedEntityView = 0;
    uint32_t m_SelectedEntityID = 0;
    int m_SelectedComponentIndex = -1;
    std::string m_SelectedComponentType;
};