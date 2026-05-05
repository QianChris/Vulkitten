#include "DefaultLayer.h"

#include "imgui.h"
#include <glm/gtc/type_ptr.hpp>

#include "Vulkitten/Scene/Entity.h"
#include "Vulkitten/Scene/SceneCamera.h"


#define VKT_TIMER(name) Vulkitten::Timer timer##__LINE__([&](float elapsed) { \
        m_ProfileResults.emplace_back(name, elapsed); });

DefaultLayer::DefaultLayer()
    : Layer("DefaultLayer"), m_CameraController(1280.0f / 720.0f)
{
    Vulkitten::FrameBufferSpecification fbSpec;
    fbSpec.Width = m_ViewportWidth;
    fbSpec.Height = m_ViewportHeight;
    m_Framebuffer = Vulkitten::FrameBuffer::Create(fbSpec);
}

void DefaultLayer::UpdateViewportFramebuffer(uint32_t width, uint32_t height)
{
    if (m_ViewportWidth != width || m_ViewportHeight != height)
    {
        m_ViewportWidth = width;
        m_ViewportHeight = height;

        Vulkitten::FrameBufferSpecification fbSpec;
        fbSpec.Width = width;
        fbSpec.Height = height;
        m_Framebuffer = Vulkitten::FrameBuffer::Create(fbSpec);
    }
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

    {
        Entity entity = m_Scene.CreateEntity("Green Quad");
        auto& transform = entity.GetComponent<TransformComponent>();
        transform.Transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, -0.5f, 0.1f)) *
                            glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.75f, 1.0f));
        entity.AddComponent<SpriteRendererComponent>(glm::vec4(0.2f, 0.8f, 0.3f, 1.0f), nullptr, 1.0f);
        m_Entities.push_back(entity);
    }

    {
        Entity entity = m_Scene.CreateEntity("Red Quad");
        auto& transform = entity.GetComponent<TransformComponent>();
        transform.Transform = glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f, 0.0f, 0.1f)) *
                            glm::scale(glm::mat4(1.0f), glm::vec3(0.75f, 0.75f, 1.0f));
        entity.AddComponent<SpriteRendererComponent>(glm::vec4(0.8f, 0.2f, 0.3f, 1.0f), nullptr, 1.0f);
        m_Entities.push_back(entity);
    }

    {
        Entity entity = m_Scene.CreateEntity("Background Quad");
        auto& transform = entity.GetComponent<TransformComponent>();
        transform.Transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)) *
                            glm::scale(glm::mat4(1.0f), glm::vec3(10.0f, 10.0f, 1.0f));
        entity.AddComponent<SpriteRendererComponent>(glm::vec4(1.0f), m_Texture, 10.0f);
        m_Entities.push_back(entity);
    }

    {
        Entity entity = m_Scene.CreateEntity("Logo");
        auto& transform = entity.GetComponent<TransformComponent>();
        transform.Transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.2f)) *
            glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f));
        entity.AddComponent<SpriteRendererComponent>(glm::vec4(1.0f), m_LogoTexture, 1.0f);
        m_Entities.push_back(entity);
    }

    {
        Entity entity = m_Scene.CreateEntity("Rotating Quad");
        auto& transform = entity.GetComponent<TransformComponent>();
        transform.Transform = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 0.2f)) *
            glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f));
        entity.AddComponent<SpriteRendererComponent>(glm::vec4(1.0f), m_Texture, 10.0f);
        m_Entities.push_back(entity);
    }
}

void DefaultLayer::OnUpdate(Vulkitten::Timestep timestep)
{
    VKT_TIMER("DefaultLayer::OnUpdate");

    m_Framebuffer->Bind();
    Vulkitten::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
    Vulkitten::RenderCommand::Clear();

    float aspectRatio = (float)m_ViewportWidth / (float)m_ViewportHeight;
    m_CameraController.GetCamera().SetProjection(-aspectRatio * m_CameraController.GetZoomLevel(),
        aspectRatio * m_CameraController.GetZoomLevel(),
        -m_CameraController.GetZoomLevel(),
        m_CameraController.GetZoomLevel());

    m_CameraController.OnUpdate(timestep);

    static float rotation = 0.0f;
    rotation -= timestep * 50.;
    auto& transform = m_Entities[4].GetComponent<Vulkitten::TransformComponent>();
    transform.Transform = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 0.2f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f));

    {
        VKT_TIMER("Render Scene");
        Vulkitten::Renderer2D::BeginScene(m_CameraController.GetCamera());
        m_Scene.OnUpdate(timestep);
        Vulkitten::Renderer2D::EndScene();
    }

    m_Framebuffer->Unbind();
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

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Viewport");
    ImGui::PopStyleVar();

    uint32_t textureID = m_Framebuffer->GetColorAttachmentRendererID();
    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    UpdateViewportFramebuffer((uint32_t)viewportPanelSize.x, (uint32_t)viewportPanelSize.y);

    ImGui::Image((void*)(intptr_t)textureID, viewportPanelSize, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
    ImGui::End();

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

void DefaultLayer::OnEvent(Vulkitten::Event& event)
{
    m_CameraController.OnEvent(event);
}