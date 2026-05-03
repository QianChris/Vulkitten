#include "Sandbox2D.h"

#include "imgui.h"
#include <glm/gtc/type_ptr.hpp>


#define VKT_TIMER(name) Vulkitten::Timer timer##__LINE__([&](float elapsed) { \
        m_ProfileResults.emplace_back(name, elapsed); });

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
    VKT_TIMER("Sandbox2D::OnUpdate");

    {
        VKT_TIMER("Render Prep");
        Vulkitten::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
        Vulkitten::RenderCommand::Clear();
    }

    m_CameraController.OnUpdate(timestep);

    {
        static float rotation = 0.f;
        rotation += timestep * 50.;

        VKT_TIMER("Render Scene");
        Vulkitten::Renderer2D::BeginScene(m_CameraController.GetCamera());
        Vulkitten::Renderer2D::DrawRotatedQuad({ 0.0f, 0.0f }, { 10.0f, 10.0f }, -30., m_Texture, 10.f, m_Color3);
        Vulkitten::Renderer2D::DrawQuad({ -1.0f, 0.0f }, { 0.8f, 0.8f }, m_Color1);
        Vulkitten::Renderer2D::DrawQuad({ 0.5f, -0.5f }, { 0.5f, 0.75f }, m_Color2);
        Vulkitten::Renderer2D::DrawRotatedQuad({ 0.0f, 0.0f }, { 1.0f, 1.0f }, 60., m_LogoTexture, 1.f, m_Color4);
        Vulkitten::Renderer2D::DrawRotatedQuad({ -2.0f, 0.0f, 0.1f}, { 1.0f, 1.0f }, rotation, m_Texture, 10.f, m_Color3);
        Vulkitten::Renderer2D::EndScene();
    }
}

void Sandbox2D::OnImguiRender()
{
    ImGui::Begin("Test");

    float fps = Vulkitten::Application::Get().GetFPS();
    float frameTime = Vulkitten::Application::Get().GetFrameTime();
    ImGui::Text("Actual FPS: %.1f", fps);
    ImGui::Text("Frame Time: %.3f ms ( FPS: %.3f )", frameTime * 1000.0f, 1. / frameTime);

auto& stats = Vulkitten::Renderer2D::GetStats();
    ImGui::Separator();
    ImGui::Text("Renderer Stats:");
    ImGui::Text("Draw Calls: %u", stats.DrawCalls);
    ImGui::Text("Quads: %u / %u", stats.Quads, Vulkitten::Renderer2D::GetMaxQuads());
    ImGui::Text("Vertices: %u", stats.Vertices);
    ImGui::Text("Texture Count: %u / %u", stats.TextureCount, Vulkitten::Renderer2D::GetMaxTextureSlots());

    ImGui::ColorEdit4("Color1", glm::value_ptr(m_Color1));
    ImGui::ColorEdit4("Color2", glm::value_ptr(m_Color2));
    ImGui::ColorEdit4("Color3", glm::value_ptr(m_Color3));
    ImGui::ColorEdit4("Color4", glm::value_ptr(m_Color4));

    //ImGui::Text("Profile Results:");
    //for (auto& result : m_ProfileResults) {
        //ImGui::Text("%s: %.3fms", result.first.c_str(), result.second);
    //}
    m_ProfileResults.clear();

    ImGui::End();
}

void Sandbox2D::OnEvent(Vulkitten::Event& event)
{
    m_CameraController.OnEvent(event);
}