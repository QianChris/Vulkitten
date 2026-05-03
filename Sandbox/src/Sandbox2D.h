#pragma once

#include <Vulkitten.h>

#include <glm/glm.hpp>
#include <entt/entt.hpp>

class Sandbox2D : public Vulkitten::Layer
{
public:
    Sandbox2D();
    virtual ~Sandbox2D() = default;

    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnUpdate(Vulkitten::Timestep timestep) override;
    virtual void OnImguiRender() override;
    virtual void OnEvent(Vulkitten::Event& event) override;

private:
    void CreateTestScene();

    Vulkitten::CameraController m_CameraController;

    Vulkitten::Ref<Vulkitten::Texture2D> m_Texture;
    Vulkitten::Ref<Vulkitten::Texture2D> m_LogoTexture;

    Vulkitten::Scene m_Scene;
    std::vector<entt::entity> m_Entities;

    std::vector<std::pair<std::string, float>> m_ProfileResults;
};