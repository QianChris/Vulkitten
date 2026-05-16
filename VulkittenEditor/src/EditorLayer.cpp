#include "EditorLayer.h"
#include "imgui.h"
#include <glm/gtc/type_ptr.hpp>

#include "Vulkitten/Scene/Entity.h"
#include "Vulkitten/Scene/SceneCamera.h"
#include "Vulkitten/Scene/SceneSerializer.h"
#include "Vulkitten/Utils/FileDialogs.h"

EditorLayer::EditorLayer()
    : Layer("EditorLayer")
    , m_Scene(Vulkitten::CreateRef<Vulkitten::Scene>())
{
}

void EditorLayer::OnAttach()
{
    m_Texture = Vulkitten::Texture2D::Create("sandbox://assets/textures/Checkerboard.png");
    m_LogoTexture = Vulkitten::Texture2D::Create("sandbox://assets/textures/ChernoLogo.png");

    // 1. 初始化 Context
    m_Context.scene = m_Scene;
    m_Context.editorCamera = &m_EditorCamera;
    m_Context.commands = &m_CommandSystem;

    // 2. 创建 Panel 并注入 Context
    m_ViewportPanel = Vulkitten::CreateScope<Vulkitten::ViewportPanel>();
    m_ViewportPanel->OnAttach(&m_Context);

    m_SceneHierarchyPanel = Vulkitten::CreateScope<Vulkitten::SceneHierarchyPanel>();
    m_SceneHierarchyPanel->OnAttach(&m_Context);

    m_PropertyPanel = Vulkitten::CreateScope<Vulkitten::PropertyPanel>();
    m_PropertyPanel->OnAttach(&m_Context);

    m_PerformancePanel = Vulkitten::CreateScope<Vulkitten::PerformancePanel>();
    m_PerformancePanel->OnAttach(&m_Context);

    m_ResourcePanel = Vulkitten::CreateScope<Vulkitten::ResourcePanel>();
    m_ResourcePanel->OnAttach(&m_Context);

    // 3. 订阅跨 Panel 信号
    m_Context.signals.Subscribe<Vulkitten::EditorEvents::EntitySelected>(
        [this](const auto& evt) {
            m_Context.selectedEntity = evt.entity;
        });

    m_Context.signals.Subscribe<Vulkitten::EditorEvents::RequestNewScene>(
        [this](const auto&) { NewScene(); });

    m_Context.signals.Subscribe<Vulkitten::EditorEvents::RequestOpenScene>(
        [this](const auto&) { OpenScene(); });

    m_Context.signals.Subscribe<Vulkitten::EditorEvents::RequestSaveScene>(
        [this](const auto&) { SaveSceneAs(); });

    CreateTestScene();

    m_EditorCamera.SetViewportSize(
        (float)m_ViewportPanel->GetViewportWidth(),
        (float)m_ViewportPanel->GetViewportHeight()
    );
}

void EditorLayer::OnDetach()
{
    m_ViewportPanel->OnDetach();
    m_SceneHierarchyPanel->OnDetach();
    m_PropertyPanel->OnDetach();
    m_PerformancePanel->OnDetach();
    m_ResourcePanel->OnDetach();
}

void EditorLayer::CreateTestScene()
{
    using namespace Vulkitten;

    {
        auto entity = m_Scene->CreateEntity("Camera");
        auto& cameraComponent = entity.AddComponent<CameraComponent>();
        cameraComponent.Primary = true;
        cameraComponent.FixedAspectRatio = false;

        float aspectRatio = (float)m_ViewportPanel->GetViewportWidth() / (float)m_ViewportPanel->GetViewportHeight();
        cameraComponent.Camera.SetOrthographicProjection(-aspectRatio * 1.0f, aspectRatio * 1.0f, -1.0f, 1.0f);

        auto& cameraTransform = entity.GetComponent<TransformComponent>();
        cameraTransform.SetPosition({ 0., 0., 1. });
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
        auto& spriteComp = entity.AddComponent<SpriteRendererComponent>(glm::vec4(1.0f), m_Texture, 10.0f);
        spriteComp.TexturePath = "sandbox://assets/textures/Checkerboard.png";
        auto& transform = entity.GetComponent<TransformComponent>();
        transform.SetPosition({ 0.0f, 0.0f, 0.0f });
        transform.SetScale({ 10.0f, 10.0f, 1.0f });
    }

    {
        Entity entity = m_Scene->CreateEntity("Logo");
        auto& spriteComp = entity.AddComponent<SpriteRendererComponent>(glm::vec4(1.0f), m_LogoTexture, 1.0f);
        spriteComp.TexturePath = "sandbox://assets/textures/ChernoLogo.png";
        auto& transform = entity.GetComponent<TransformComponent>();
        transform.SetPosition({ 0.0f, 0.0f, 0.2f });
        transform.SetScale({ 1.0f, 1.0f, 1.0f });
    }

    {
        Entity entity = m_Scene->CreateEntity("Rotating Quad");
        auto& spriteComp = entity.AddComponent<SpriteRendererComponent>(glm::vec4(1.0f), m_Texture, 10.0f);
        spriteComp.TexturePath = "sandbox://assets/textures/Checkerboard.png";
        auto& transform = entity.GetComponent<TransformComponent>();
        transform.SetPosition({ -2.0f, 0.0f, 0.2f });
        transform.SetScale({ 1.0f, 1.0f, 1.0f });
    }
}

// ═════════════════════════════════════════════════════════════
// OnUpdate：逻辑 + 渲染到 Viewport Framebuffer（无 ImGui）
// ═════════════════════════════════════════════════════════════
void EditorLayer::OnUpdate(Vulkitten::Timestep timestep)
{
    // Phase 1: 全局输入（EditorCamera 漫游）
    if (IsViewportFocused())
        m_EditorCamera.OnUpdate(timestep);

    // Phase 2: 各 Panel 逻辑更新（无 UI）
    m_ViewportPanel->OnUpdate(timestep);
    m_SceneHierarchyPanel->OnUpdate(timestep);
    m_PropertyPanel->OnUpdate(timestep);
    m_PerformancePanel->OnUpdate(timestep);
    m_ResourcePanel->OnUpdate(timestep);

    // Phase 3: 同步相机比例
    if (m_ViewportPanel->GetViewportWidth() > 0 && m_ViewportPanel->GetViewportHeight() > 0)
    {
        float aspect = (float)m_ViewportPanel->GetViewportWidth() / (float)m_ViewportPanel->GetViewportHeight();
        m_Scene->SetCameraAspectRatio(aspect);
        m_EditorCamera.SetViewportSize(
            (float)m_ViewportPanel->GetViewportWidth(),
            (float)m_ViewportPanel->GetViewportHeight()
        );
    }

    // Phase 4: 渲染场景到 Viewport Framebuffer
    auto framebuffer = m_ViewportPanel->GetFramebuffer();
    framebuffer->Bind();
    Vulkitten::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
    Vulkitten::RenderCommand::Clear();
    framebuffer->ClearAttachment(1, -1);
    m_Scene->OnUpdate(timestep);
    framebuffer->Unbind();
}

// ═════════════════════════════════════════════════════════════
// OnImguiRender：纯 UI，不改 ECS 状态（只发 Signal）
// ═════════════════════════════════════════════════════════════
void EditorLayer::OnImguiRender()
{
    static bool dockspaceOpen = true;
    static bool opt_fullscreen_persistant = true;
    bool opt_fullscreen = opt_fullscreen_persistant;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

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

    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
    ImGui::PopStyleVar();
    if (opt_fullscreen)
        ImGui::PopStyleVar(2);

    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

    // MenuBar
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New", "Ctrl+N"))
                m_Context.signals.Publish<Vulkitten::EditorEvents::RequestNewScene>();
            if (ImGui::MenuItem("Open...", "Ctrl+O"))
                m_Context.signals.Publish<Vulkitten::EditorEvents::RequestOpenScene>();
            ImGui::Separator();
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
                m_Context.signals.Publish<Vulkitten::EditorEvents::RequestSaveScene>();
            ImGui::Separator();
            if (ImGui::MenuItem("Exit"))
                Vulkitten::Application::Get().SetClose();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, m_CommandSystem.CanUndo()))
                m_CommandSystem.Undo();
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, m_CommandSystem.CanRedo()))
                m_CommandSystem.Redo();
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    // Phase: 各 Panel UI 渲染
    m_ViewportPanel->OnUIRender();
    m_SceneHierarchyPanel->OnUIRender();
    m_PropertyPanel->OnUIRender();
    m_ResourcePanel->OnUIRender();
    m_PerformancePanel->OnUIRender();

    ImGui::End();
    ImGui::ShowDemoWindow();
}

void EditorLayer::OnEvent(Vulkitten::Event& event)
{
    if (m_ViewportPanel->IsFocusedAndHovered())
        m_EditorCamera.OnEvent(event);

    m_ViewportPanel->OnEvent(event);
    m_SceneHierarchyPanel->OnEvent(event);
    m_PropertyPanel->OnEvent(event);

    Vulkitten::EventDispatcher dispatcher(event);
    dispatcher.Dispatch<Vulkitten::KeyPressedEvent>(VKT_BIND_EVENT_FN(EditorLayer::OnKeyPressed));
    dispatcher.Dispatch<Vulkitten::MouseButtonPressedEvent>(VKT_BIND_EVENT_FN(EditorLayer::OnMouseButtonPressed));
}

bool EditorLayer::OnKeyPressed(Vulkitten::KeyPressedEvent& event)
{
    bool ctrl  = Vulkitten::Input::IsKeyPressed(VKT_KEY_LEFT_CONTROL) || Vulkitten::Input::IsKeyPressed(VKT_KEY_RIGHT_CONTROL);
    bool shift = Vulkitten::Input::IsKeyPressed(VKT_KEY_LEFT_SHIFT)    || Vulkitten::Input::IsKeyPressed(VKT_KEY_RIGHT_SHIFT);

    if (ctrl && event.GetKeyCode() == VKT_KEY_N) {
        m_Context.signals.Publish<Vulkitten::EditorEvents::RequestNewScene>();
        return true;
    }
    else if (ctrl && event.GetKeyCode() == VKT_KEY_O) {
        m_Context.signals.Publish<Vulkitten::EditorEvents::RequestOpenScene>();
        return true;
    }
    else if (ctrl && shift && event.GetKeyCode() == VKT_KEY_S) {
        m_Context.signals.Publish<Vulkitten::EditorEvents::RequestSaveScene>();
        return true;
    }
    else if (ctrl && !shift && event.GetKeyCode() == VKT_KEY_Z) {
        m_CommandSystem.Undo();
        return true;
    }
    else if ((ctrl && event.GetKeyCode() == VKT_KEY_Y) || (ctrl && shift && event.GetKeyCode() == VKT_KEY_Z)) {
        m_CommandSystem.Redo();
        return true;
    }
    return false;
}

bool EditorLayer::OnMouseButtonPressed(Vulkitten::MouseButtonPressedEvent& event)
{
    if (m_ViewportPanel->IsFocusedAndHovered() && event.GetMouseButton() == VKT_MOUSE_BUTTON_LEFT)
        return true;
    return false;
}

void EditorLayer::NewScene()
{
    m_Scene = Vulkitten::CreateRef<Vulkitten::Scene>();
    m_Context.scene = m_Scene;
    m_Context.selectedEntity = Vulkitten::Entity{};
    m_Context.hoveredEntity = Vulkitten::Entity{};
    m_CommandSystem.Clear();

    m_ViewportPanel->OnAttach(&m_Context);
    m_SceneHierarchyPanel->OnAttach(&m_Context);
    m_PropertyPanel->OnAttach(&m_Context);
    m_PerformancePanel->OnAttach(&m_Context);
    m_ResourcePanel->OnAttach(&m_Context);

    m_CurrentScenePath.clear();
    CreateTestScene();
}

void EditorLayer::OpenScene()
{
    static Vulkitten::FileDialogs fileDialogs;
    std::string filepath = fileDialogs.OpenFile("YAML Files (*.yaml)\0*.yaml\0All Files (*.*)\0*.*\0");
    if (!filepath.empty())
    {
        m_Scene = Vulkitten::CreateRef<Vulkitten::Scene>();
        Vulkitten::SceneSerializer serializer(m_Scene);
        if (serializer.Deserialize(filepath))
        {
            m_Context.scene = m_Scene;
            m_Context.selectedEntity = Vulkitten::Entity{};
            m_Context.hoveredEntity = Vulkitten::Entity{};
            m_CommandSystem.Clear();

            m_ViewportPanel->OnAttach(&m_Context);
            m_SceneHierarchyPanel->OnAttach(&m_Context);
            m_PropertyPanel->OnAttach(&m_Context);
            m_PerformancePanel->OnAttach(&m_Context);
            m_ResourcePanel->OnAttach(&m_Context);

            m_CurrentScenePath = filepath;
        }
    }
}

void EditorLayer::SaveSceneAs()
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

bool EditorLayer::IsViewportFocused() const
{
    return m_ViewportPanel->IsFocusedAndHovered();
}