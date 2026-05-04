#include "DefaultLayer.h"

#include "imgui.h"
#include <glm/gtc/type_ptr.hpp>


#define VKT_TIMER(name) Vulkitten::Timer timer##__LINE__([&](float elapsed) { \
        m_ProfileResults.emplace_back(name, elapsed); });

DefaultLayer::DefaultLayer()
    : Layer("DefaultLayer"), m_CameraController(1280.0f / 720.0f)
{
}

void DefaultLayer::OnAttach()
{
    m_Texture = Vulkitten::Texture2D::Create("sandbox://assets/textures/Checkerboard.png");
    m_LogoTexture = Vulkitten::Texture2D::Create("sandbox://assets/textures/ChernoLogo.png");

    CreateTestScene();
}

void DefaultLayer::OnDetach()
{
}

void DefaultLayer::CreateTestScene()
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

void DefaultLayer::OnUpdate(Vulkitten::Timestep timestep)
{
    VKT_TIMER("DefaultLayer::OnUpdate");

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

void DefaultLayer::OnImguiRender()
{
    // Note: Switch this to true to enable dockspace
    static bool dockspaceOpen = true;
    static bool opt_fullscreen_persistant = true;
    bool opt_fullscreen = opt_fullscreen_persistant;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }

    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
    ImGui::PopStyleVar();
    
    if (opt_fullscreen)
        ImGui::PopStyleVar(2);

    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Files"))
        {
            if (ImGui::MenuItem("Exit"))
            {
                Vulkitten::Application::Get().SetClose();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGuiTest();

    ImGui::End();
}

void DefaultLayer::ImGuiTest() {

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

void DefaultLayer::OnEvent(Vulkitten::Event& event)
{
    m_CameraController.OnEvent(event);
}