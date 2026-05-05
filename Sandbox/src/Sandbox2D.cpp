#include "Sandbox2D.h"

#include "imgui.h"
#include <glm/gtc/type_ptr.hpp>

#include "Vulkitten/Scene/Entity.h"
#include "Vulkitten/Scene/SceneCamera.h"


#define VKT_TIMER(name) Vulkitten::Timer timer##__LINE__([&](float elapsed) { \
        m_ProfileResults.emplace_back(name, elapsed); });

Sandbox2D::Sandbox2D()
    : Layer("Sandbox2D")
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

    {
        Entity cameraEntity = m_Scene.CreateEntity("Camera");
        cameraEntity.AddComponent<TransformComponent>();
        auto& cameraComponent = cameraEntity.AddComponent<CameraComponent>();
        cameraComponent.Primary = true;
        cameraComponent.Camera.SetOrthographicProjection(-5.0f, 5.0f, -5.0f, 5.0f);
    }

    {
        Entity entity1 = m_Scene.CreateEntity("Red Quad");
        entity1.AddComponent<SpriteRendererComponent>();
        auto& sprite1 = entity1.GetComponent<SpriteRendererComponent>();
        sprite1.Color = { 0.8f, 0.2f, 0.3f, 1.0f };
        sprite1.Texture = nullptr;
        sprite1.TilingFactor = 1.0f;
        auto& transform1 = entity1.GetComponent<TransformComponent>();
        transform1.SetPosition({ -1.0f, 0.0f, 0.1f });
        transform1.SetScale({ 0.8f, 0.8f, 1.0f });
        m_Entities.push_back(entity1);
    }

    {
        Entity entity2 = m_Scene.CreateEntity("Green Quad");
        entity2.AddComponent<SpriteRendererComponent>();
        auto& sprite2 = entity2.GetComponent<SpriteRendererComponent>();
        sprite2.Color = { 0.2f, 0.8f, 0.3f, 1.0f };
        sprite2.Texture = nullptr;
        sprite2.TilingFactor = 1.0f;
        auto& transform2 = entity2.GetComponent<TransformComponent>();
        transform2.SetPosition({ 0.5f, -0.5f, 0.1f });
        transform2.SetScale({ 0.5f, 0.75f, 1.0f });
        m_Entities.push_back(entity2);
    }

    {
        Entity entity3 = m_Scene.CreateEntity("Background Quad");
        entity3.AddComponent<SpriteRendererComponent>();
        auto& sprite3 = entity3.GetComponent<SpriteRendererComponent>();
        sprite3.Color = { 1.0f, 1.0f, 1.0f, 1.0f };
        sprite3.Texture = m_Texture;
        sprite3.TilingFactor = 10.0f;
        auto& transform3 = entity3.GetComponent<TransformComponent>();
        transform3.SetPosition({ 0.0f, 0.0f, 0.0f });
        transform3.SetScale({ 10.0f, 10.0f, 1.0f });
        m_Entities.push_back(entity3);
    }

    {
        Entity entity4 = m_Scene.CreateEntity("Logo");
        entity4.AddComponent<SpriteRendererComponent>();
        auto& sprite4 = entity4.GetComponent<SpriteRendererComponent>();
        sprite4.Color = { 1.0f, 1.0f, 1.0f, 1.0f };
        sprite4.Texture = m_LogoTexture;
        sprite4.TilingFactor = 1.0f;
        auto& transform4 = entity4.GetComponent<TransformComponent>();
        transform4.SetPosition({ 0.0f, 0.0f, 0.2f });
        transform4.SetScale({ 1.0f, 1.0f, 1.0f });
        m_Entities.push_back(entity4);
    }

    {
        Entity entity5 = m_Scene.CreateEntity("Rotating Quad");
        entity5.AddComponent<SpriteRendererComponent>();
        auto& sprite5 = entity5.GetComponent<SpriteRendererComponent>();
        sprite5.Color = { 1.0f, 1.0f, 1.0f, 1.0f };
        sprite5.Texture = m_Texture;
        sprite5.TilingFactor = 10.0f;
        auto& transform5 = entity5.GetComponent<TransformComponent>();
        transform5.SetPosition({ -2.0f, 0.0f, 0.2f });
        transform5.SetScale({ 1.0f, 1.0f, 1.0f });
        m_Entities.push_back(entity5);
    }
}

void Sandbox2D::OnUpdate(Vulkitten::Timestep timestep)
{
    VKT_TIMER("Sandbox2D::OnUpdate");

    {
        VKT_TIMER("Render Prep");
        Vulkitten::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
        Vulkitten::RenderCommand::Clear();
    }

    static float rotation = 0.0f;
    rotation -= timestep * 50.0f;
    auto& transform = m_Entities[4].GetComponent<Vulkitten::TransformComponent>();
    transform.SetPosition({ -2.0f, 0.0f, 0.2f });
    transform.SetRotation({ 0.0f, 0.0f, rotation });
    transform.SetScale({ 1.0f, 1.0f, 1.0f });

    {
        VKT_TIMER("Render Scene");
        m_Scene.OnUpdate(timestep);
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
    ImGui::ColorEdit4("Color1", glm::value_ptr(m_Entities[0].GetComponent<Vulkitten::SpriteRendererComponent>().Color));
    ImGui::ColorEdit4("Color2", glm::value_ptr(m_Entities[1].GetComponent<Vulkitten::SpriteRendererComponent>().Color));
    ImGui::ColorEdit4("Color3", glm::value_ptr(m_Entities[2].GetComponent<Vulkitten::SpriteRendererComponent>().Color));
    ImGui::ColorEdit4("Color4", glm::value_ptr(m_Entities[3].GetComponent<Vulkitten::SpriteRendererComponent>().Color));
    ImGui::ColorEdit4("Color5", glm::value_ptr(m_Entities[4].GetComponent<Vulkitten::SpriteRendererComponent>().Color));

    //ImGui::Text("Profile Results:");
    //for (auto& result : m_ProfileResults) {
        //ImGui::Text("%s: %.3fms", result.first.c_str(), result.second);
    //}
    m_ProfileResults.clear();

    ImGui::End();
}

void Sandbox2D::OnEvent(Vulkitten::Event& event)
{
}