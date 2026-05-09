#include "DefaultLayer.h"

#include "imgui.h"
#include <glm/gtc/type_ptr.hpp>

#include "Vulkitten/Scene/Entity.h"
#include "Vulkitten/Scene/SceneCamera.h"
#include "Vulkitten/Scene/SceneSerializer.h"
#include "Vulkitten/Utils/FileDialogs.h"


#define VKT_TIMER(name) Vulkitten::Timer timer##__LINE__([&](float elapsed) { \
        m_ProfileResults.emplace_back(name, elapsed); });

DefaultLayer::DefaultLayer()
    : Layer("DefaultLayer")
    , m_Scene(Vulkitten::CreateRef<Vulkitten::Scene>())
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

m_SceneHierarchyPanel.SetContext(m_Scene);
    m_PropertyPanel.SetContext(m_Scene);
    m_PerformancePanel.SetContext(m_Scene);
}

void DefaultLayer::OnDetach()
{
}

void DefaultLayer::CreateTestScene()
{
    using namespace Vulkitten;

    class CameraController : public ScriptableEntity
    {
        void OnUpdate(Timestep ts) override
        {
            float moveSpeed = 1.0f;
            float rotationSpeed = 50.0f;
            if (Input::IsKeyPressed(VKT_KEY_W))
                GetComponent<TransformComponent>().SetDeltaPosition({ 0.0f, ts * moveSpeed, 0.0f });
			if (Input::IsKeyPressed(VKT_KEY_S))
                GetComponent<TransformComponent>().SetDeltaPosition({ 0.0f, -ts * moveSpeed, 0.0f });
            if (Input::IsKeyPressed(VKT_KEY_A))
                GetComponent<TransformComponent>().SetDeltaPosition({ -ts * moveSpeed, 0.0f, 0.0f });
            if (Input::IsKeyPressed(VKT_KEY_D))
                GetComponent<TransformComponent>().SetDeltaPosition({ ts * moveSpeed, 0.0f, 0.0f });
            if (Input::IsKeyPressed(VKT_KEY_Q))
                GetComponent<TransformComponent>().SetDeltaRotation({ 0.0f, 0.0f, ts * rotationSpeed });
            if (Input::IsKeyPressed(VKT_KEY_E))
                GetComponent<TransformComponent>().SetDeltaRotation({ 0.0f, 0.0f, -ts * rotationSpeed });
        }
    };

    {
        auto entity = m_Scene->CreateEntity("Camera");
        auto& cameraComponent = entity.AddComponent<CameraComponent>();
        cameraComponent.Primary = true;
        cameraComponent.FixedAspectRatio = false;

        float aspectRatio = (float)m_ViewportWidth / (float)m_ViewportHeight;
        cameraComponent.Camera.SetOrthographicProjection(-aspectRatio * 1.0f, aspectRatio * 1.0f, -1.0f, 1.0f);

        entity.AddComponent<NativeScriptComponent>().Bind<CameraController>();
    }

    {
        Entity entity = m_Scene->CreateEntity("Green Quad");
        entity.AddComponent<SpriteRendererComponent>(glm::vec4(0.2f, 0.8f, 0.3f, 1.0f), nullptr, 1.0f);
        auto& transform = entity.GetComponent<TransformComponent>();
        transform.SetPosition({ 0.5f, -0.5f, 0.1f });
        transform.SetScale({ 0.5f, 0.75f, 1.0f });
    }

    {
        Entity entity = m_Scene->CreateEntity("Red Quad");
        entity.AddComponent<SpriteRendererComponent>(glm::vec4(0.8f, 0.2f, 0.3f, 1.0f), nullptr, 1.0f);
        auto& transform = entity.GetComponent<TransformComponent>();
        transform.SetPosition({ -0.5f, 0.0f, 0.1f });
        transform.SetScale({ 0.75f, 0.75f, 1.0f });
    }

    {
        Entity entity = m_Scene->CreateEntity("Background Quad");
        entity.AddComponent<SpriteRendererComponent>(glm::vec4(1.0f), m_Texture, 10.0f);
        auto& transform = entity.GetComponent<TransformComponent>();
        transform.SetPosition({ 0.0f, 0.0f, 0.0f });
        transform.SetScale({ 10.0f, 10.0f, 1.0f });
    }

    {
        Entity entity = m_Scene->CreateEntity("Logo");
        entity.AddComponent<SpriteRendererComponent>(glm::vec4(1.0f), m_LogoTexture, 1.0f);
        auto& transform = entity.GetComponent<TransformComponent>();
        transform.SetPosition({ 0.0f, 0.0f, 0.2f });
        transform.SetScale({ 1.0f, 1.0f, 1.0f });
    }

    {
        Entity entity = m_Scene->CreateEntity("Rotating Quad");
        entity.AddComponent<SpriteRendererComponent>(glm::vec4(1.0f), m_Texture, 10.0f);
        auto& transform = entity.GetComponent<TransformComponent>();
        transform.SetPosition({ -2.0f, 0.0f, 0.2f });
        transform.SetScale({ 1.0f, 1.0f, 1.0f });
    }
}

void DefaultLayer::OnUpdate(Vulkitten::Timestep timestep)
{
    VKT_TIMER("DefaultLayer::OnUpdate");

    m_Framebuffer->Bind();
    Vulkitten::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
    Vulkitten::RenderCommand::Clear();

    {
        float aspectRatio = (float)m_ViewportWidth / (float)m_ViewportHeight;
        m_Scene->SetCameraAspectRatio(aspectRatio);
    }

    {
        VKT_TIMER("Render Scene");
        m_Scene->OnUpdate(timestep);
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
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New", "Ctrl+N"))
            {
                NewScene();
            }
            if (ImGui::MenuItem("Open...", "Ctrl+O"))
            {
                OpenScene();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
            {
                SaveSceneAs();
            }
            ImGui::Separator();
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

    m_SceneHierarchyPanel.OnImGuiRender();
    m_PropertyPanel.SetSelectedEntity(m_SceneHierarchyPanel.GetSelectedEntity());
    m_PropertyPanel.OnImGuiRender();
    m_PerformancePanel.OnImGuiRender();

    ImGui::End();

    ImGui::ShowDemoWindow();
}

void DefaultLayer::OnEvent(Vulkitten::Event& event)
{
    Vulkitten::EventDispatcher dispatcher(event);
    dispatcher.Dispatch<Vulkitten::KeyPressedEvent>(VKT_BIND_EVENT_FN(DefaultLayer::OnKeyPressed));
}

bool DefaultLayer::OnKeyPressed(Vulkitten::KeyPressedEvent& event)
{
    bool ctrlState = Vulkitten::Input::IsKeyPressed(VKT_KEY_LEFT_CONTROL) || Vulkitten::Input::IsKeyPressed(VKT_KEY_RIGHT_CONTROL);
    bool shiftState = Vulkitten::Input::IsKeyPressed(VKT_KEY_LEFT_SHIFT) || Vulkitten::Input::IsKeyPressed(VKT_KEY_RIGHT_SHIFT);

    if (ctrlState && event.GetKeyCode() == VKT_KEY_N)
    {
        NewScene();
        return true;
    }
    else if (ctrlState && event.GetKeyCode() == VKT_KEY_O)
    {
        OpenScene();
        return true;
    }
    else if (ctrlState && shiftState && event.GetKeyCode() == VKT_KEY_S)
    {
        SaveSceneAs();
        return true;
    }

    return false;
}

void DefaultLayer::NewScene()
{
    m_Scene = Vulkitten::CreateRef<Vulkitten::Scene>();
    m_SceneHierarchyPanel.SetContext(m_Scene);
    m_PropertyPanel.SetContext(m_Scene);
    m_PerformancePanel.SetContext(m_Scene);
    m_CurrentScenePath.clear();
    CreateTestScene();
}

void DefaultLayer::OpenScene()
{
    static Vulkitten::FileDialogs fileDialogs;
    std::string filepath = fileDialogs.OpenFile("YAML Files (*.yaml)\0*.yaml\0All Files (*.*)\0*.*\0");
    if (!filepath.empty())
    {
        m_Scene = Vulkitten::CreateRef<Vulkitten::Scene>();
        Vulkitten::SceneSerializer serializer(m_Scene);
        if (serializer.Deserialize(filepath))
        {
            m_SceneHierarchyPanel.SetContext(m_Scene);
            m_PropertyPanel.SetContext(m_Scene);
            m_PerformancePanel.SetContext(m_Scene);
            m_CurrentScenePath = filepath;
        }
    }
}

void DefaultLayer::SaveSceneAs()
{
    static Vulkitten::FileDialogs fileDialogs;
    std::string filepath = fileDialogs.SaveFile("YAML Files (*.yaml)\0*.yaml\0All Files (*.*)\0*.*\0");
    if (!filepath.empty())
    {
        Vulkitten::SceneSerializer serializer(m_Scene);
        serializer.Serialize(filepath);
        m_CurrentScenePath = filepath;
    }
}