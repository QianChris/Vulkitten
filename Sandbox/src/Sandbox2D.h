#pragma once

#include <Vulkitten.h>

#include <glm/glm.hpp>

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
    Vulkitten::CameraController m_CameraController;

    Vulkitten::Ref<Vulkitten::Texture2D> m_Texture;
    Vulkitten::Ref<Vulkitten::Texture2D> m_LogoTexture;

    std::vector<std::pair<std::string, float>> m_ProfileResults;

    glm::vec4 m_Color1{ 0.8f, 0.2f, 0.3f, 1.0f };
    glm::vec4 m_Color2{ 0.3f, 0.2f, 0.8f, 1.0f };
    glm::vec4 m_Color3{ 1.0f, 1.0f, 1.0f, 1.0f };
    glm::vec4 m_Color4{ 1.0f, 1.0f, 1.0f, 1.0f };
};