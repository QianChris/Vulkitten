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

    CreateTestScene();
}

void Sandbox2D::OnDetach()
{
}

void Sandbox2D::CreateTestScene()
{
    using namespace Vulkitten;

    auto entity4 = m_Scene.CreateEntity();
    m_Scene.GetRegistry().emplace<TransformComponent>(entity4);
    m_Scene.GetRegistry().emplace<SpriteRendererComponent>(entity4);
    auto& transform4 = m_Scene.GetRegistry().get<TransformComponent>(entity4);
    auto& sprite4 = m_Scene.GetRegistry().get<SpriteRendererComponent>(entity4);
    sprite4.Color = { 1.0f, 1.0f, 1.0f, 1.0f };
    sprite4.Texture = m_LogoTexture;
    sprite4.TilingFactor = 1.0f;
    transform4.Transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.2f)) *
                       glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f));

    auto entity5 = m_Scene.CreateEntity();
    m_Scene.GetRegistry().emplace<TransformComponent>(entity5);
    m_Scene.GetRegistry().emplace<SpriteRendererComponent>(entity5);
    auto& transform5 = m_Scene.GetRegistry().get<TransformComponent>(entity5);
    auto& sprite5 = m_Scene.GetRegistry().get<SpriteRendererComponent>(entity5);
    sprite5.Color = { 1.0f, 1.0f, 1.0f, 1.0f };
    sprite5.Texture = m_Texture;
    sprite5.TilingFactor = 10.0f;
    transform5.Transform = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 0.2f)) *
                       glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f));

    auto entity1 = m_Scene.CreateEntity();
    m_Scene.GetRegistry().emplace<TransformComponent>(entity1);
    m_Scene.GetRegistry().emplace<SpriteRendererComponent>(entity1);
    auto& transform1 = m_Scene.GetRegistry().get<TransformComponent>(entity1);
    auto& sprite1 = m_Scene.GetRegistry().get<SpriteRendererComponent>(entity1);
    sprite1.Color = { 0.8f, 0.2f, 0.3f, 1.0f };
    sprite1.Texture = nullptr;
    sprite1.TilingFactor = 1.0f;
    transform1.Transform = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.0f, 0.1f)) *
                       glm::scale(glm::mat4(1.0f), glm::vec3(0.8f, 0.8f, 1.0f));

    auto entity2 = m_Scene.CreateEntity();
    m_Scene.GetRegistry().emplace<TransformComponent>(entity2);
    m_Scene.GetRegistry().emplace<SpriteRendererComponent>(entity2);
    auto& transform2 = m_Scene.GetRegistry().get<TransformComponent>(entity2);
    auto& sprite2 = m_Scene.GetRegistry().get<SpriteRendererComponent>(entity2);
    sprite2.Color = { 0.2f, 0.8f, 0.3f, 1.0f };
    sprite2.Texture = nullptr;
    sprite2.TilingFactor = 1.0f;
    transform2.Transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, -0.5f, 0.1f)) *
                       glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.75f, 1.0f));

    auto entity3 = m_Scene.CreateEntity();
    m_Scene.GetRegistry().emplace<TransformComponent>(entity3);
    m_Scene.GetRegistry().emplace<SpriteRendererComponent>(entity3);
    auto& transform3 = m_Scene.GetRegistry().get<TransformComponent>(entity3);
    auto& sprite3 = m_Scene.GetRegistry().get<SpriteRendererComponent>(entity3);
    sprite3.Color = { 1.0f, 1.0f, 1.0f, 1.0f };
    sprite3.Texture = m_Texture;
    sprite3.TilingFactor = 10.0f;
    transform3.Transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)) *
                       glm::scale(glm::mat4(1.0f), glm::vec3(10.0f, 10.0f, 1.0f));

    m_Entities.push_back(entity1);
    m_Entities.push_back(entity2);
    m_Entities.push_back(entity3);
    m_Entities.push_back(entity4);
    m_Entities.push_back(entity5);
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

    static float rotation = 0.0f;
    rotation -= timestep * 50.;
    auto& transform = m_Scene.GetRegistry().get<Vulkitten::TransformComponent>(m_Entities[4]);
    transform.Transform = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 0.2f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f));

    {
        VKT_TIMER("Render Scene");
        Vulkitten::Renderer2D::BeginScene(m_CameraController.GetCamera());
        m_Scene.OnUpdate(timestep);
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

    ImGui::Text("Color control");
    auto& reg = m_Scene.GetRegistry();
    ImGui::ColorEdit4("Color1", glm::value_ptr(reg.get<Vulkitten::SpriteRendererComponent>(m_Entities[0]).Color));
    ImGui::ColorEdit4("Color2", glm::value_ptr(reg.get<Vulkitten::SpriteRendererComponent>(m_Entities[1]).Color));
    ImGui::ColorEdit4("Color3", glm::value_ptr(reg.get<Vulkitten::SpriteRendererComponent>(m_Entities[2]).Color));
    ImGui::ColorEdit4("Color4", glm::value_ptr(reg.get<Vulkitten::SpriteRendererComponent>(m_Entities[3]).Color));
    ImGui::ColorEdit4("Color5", glm::value_ptr(reg.get<Vulkitten::SpriteRendererComponent>(m_Entities[4]).Color));

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