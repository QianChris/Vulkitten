#include "Sandbox2D.h"

#include "imgui.h"
#include <glm/gtc/type_ptr.hpp>

Sandbox2D::Sandbox2D()
    : Layer("Sandbox2D"), m_CameraController(1280.0f / 720.0f)
{
}

void Sandbox2D::OnAttach()
{
    m_Texture = Vulkitten::Texture2D::Create("sandbox://assets/textures/Checkerboard.png");
    m_LogoTexture = Vulkitten::Texture2D::Create("sandbox://assets/textures/ChernoLogo.png");
}

void Sandbox2D::OnDetach()
{
}

void Sandbox2D::OnUpdate(Vulkitten::Timestep timestep)
{
    Vulkitten::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
    Vulkitten::RenderCommand::Clear();

    m_CameraController.OnUpdate(timestep);

    Vulkitten::Renderer2D::BeginScene(m_CameraController.GetCamera());
    Vulkitten::Renderer2D::DrawQuad({ -1.0f, 0.0f }, { 0.8f, 0.8f }, m_Color1);
    Vulkitten::Renderer2D::DrawQuad({ 0.5f, -0.5f }, { 0.5f, 0.75f }, m_Color2);
    Vulkitten::Renderer2D::DrawQuad({ 0.0f, 0.0f }, { 1.0f, 1.0f }, m_Texture, m_Color3);
    Vulkitten::Renderer2D::DrawQuad({ 0.0f, 0.0f }, { 1.0f, 1.0f }, m_LogoTexture, m_Color4);
    Vulkitten::Renderer2D::EndScene();
}

void Sandbox2D::OnImguiRender()
{
    ImGui::Begin("Test");
    ImGui::ColorEdit4("Color1", glm::value_ptr(m_Color1));
    ImGui::ColorEdit4("Color2", glm::value_ptr(m_Color2));
    ImGui::ColorEdit4("Color3", glm::value_ptr(m_Color3));
    ImGui::ColorEdit4("Color4", glm::value_ptr(m_Color4));
    ImGui::End();
}

void Sandbox2D::OnEvent(Vulkitten::Event& event)
{
}