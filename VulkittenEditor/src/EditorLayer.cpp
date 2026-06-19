#include "EditorLayer.h"

#include "imgui.h"
#include <glm/gtc/type_ptr.hpp>

#include "Vulkitten/Core/Application.h"
#include "Vulkitten/Core/KeyCode.h"
#include "Vulkitten/Core/MouseButtonCode.h"
#include "Vulkitten/Core/Input.h"
#include "Vulkitten/Renderer/RenderCommand.h"
#include "Vulkitten/Renderer/RenderContext.h"
#include "Vulkitten/Renderer/Renderer.h"
#include "Vulkitten/Scene/Entity.h"
#include "Vulkitten/Scene/Components.h"
#include "Vulkitten/Scene/SceneCamera.h"
#include "Vulkitten/Scene/SceneSerializer.h"
#include "Vulkitten/Utils/FileDialogs.h"

namespace Vulkitten {

    EditorLayer::EditorLayer()
        : Layer("EditorLayer")
        , m_Scene(CreateRef<Scene>())
    {
    }

    void EditorLayer::OnAttach()
    {
        m_Texture = Texture2D::Create("sandbox://assets/textures/Checkerboard.png");
        m_LogoTexture = Texture2D::Create("sandbox://assets/textures/ChernoLogo.png");
		m_IconPlay = Texture2D::Create("editorIcons://PlayButton.png");
		m_IconPause = Texture2D::Create("editorIcons://PauseButton.png");
		m_IconStop = Texture2D::Create("editorIcons://StopButton.png");
		m_IconSimulate = Texture2D::Create("editorIcons://SimulateButton.png");
		m_IconStep = Texture2D::Create("editorIcons://StepButton.png");

        // 1. 初始化 Context
        m_Context.scene = m_Scene;
        m_Context.editorCamera = &m_EditorCamera;
        m_Context.commands = &m_CommandSystem;

        // 2. 创建 Panel 并注入 Context
        m_ViewportPanel = CreateScope<ViewportPanel>();
        m_ViewportPanel->OnAttach(&m_Context);

        m_SceneHierarchyPanel = CreateScope<SceneHierarchyPanel>();
        m_SceneHierarchyPanel->OnAttach(&m_Context);

        m_PropertyPanel = CreateScope<PropertyPanel>();
        m_PropertyPanel->OnAttach(&m_Context);

        m_PerformancePanel = CreateScope<PerformancePanel>();
        m_PerformancePanel->OnAttach(&m_Context);

        m_ResourcePanel = CreateScope<ResourcePanel>();
        m_ResourcePanel->OnAttach(&m_Context);

        // 3. 订阅跨 Panel 信号
        m_Context.signals.Subscribe<EditorEvents::EntitySelected>(
            [this](const auto& evt) {
                m_Context.selectedEntity = evt.entity;
            });

        m_Context.signals.Subscribe<EditorEvents::RequestNewScene>(
            [this](const auto&) { NewScene(); });

        m_Context.signals.Subscribe<EditorEvents::RequestOpenScene>(
            [this](const auto&) { OpenScene(); });

        m_Context.signals.Subscribe<EditorEvents::RequestSaveScene>(
            [this](const auto&) { SaveSceneAs(); });

        // 4. 
        CreateTestScene();

        // 5. Set up editor camera
        m_EditorCamera.SetViewportSize(
            (float)m_ViewportPanel->GetViewportWidth(),
            (float)m_ViewportPanel->GetViewportHeight()
        );
        m_EditorCamera.OnUpdate(0.0f);
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

        {
			Entity entity = m_Scene->CreateEntity("ParticleEmitter");
			auto& particleComp = entity.AddComponent<GpuEmitterComponent>(m_LogoTexture, 10000u);
        }
    }

    // ═════════════════════════════════════════════════════════════
    // OnUpdate：逻辑 + 渲染到 Viewport Framebuffer（无 ImGui）
    // ═════════════════════════════════════════════════════════════
    void EditorLayer::OnUpdate(Timestep timestep)
    {
        // Phase 1: 全局输入（EditorCamera 漫游）
        if (IsViewportFocused() && m_Context.isEditorCameraActive)
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

        // Phase 4: 渲染场景到 Viewport Framebuffer (via RenderGraph)
        auto framebuffer = m_ViewportPanel->GetFramebuffer();
        RenderContext::Get().GetRenderGraph()->SetFramebuffer(framebuffer);

        // Clear entity ID attachment (editor-specific, not yet in RenderGraph)
        framebuffer->Bind();
        framebuffer->ClearAttachment(1, -1);
        framebuffer->Unbind();

        if (m_Context.isEditorCameraActive)
            m_Scene->SetEditorCamera(&m_EditorCamera);
        else
            m_Scene->SetEditorCamera(nullptr);

        m_Scene->OnUpdate(timestep);
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
                    m_Context.signals.Publish<EditorEvents::RequestNewScene>();
                if (ImGui::MenuItem("Open...", "Ctrl+O"))
                    m_Context.signals.Publish<EditorEvents::RequestOpenScene>();
                ImGui::Separator();
                if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
                    m_Context.signals.Publish<EditorEvents::RequestSaveScene>();
                ImGui::Separator();
                if (ImGui::MenuItem("Exit"))
                    Application::Get().SetClose();
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

        UI_Toolbar();

        // Phase: 各 Panel UI 渲染
        m_ViewportPanel->OnUIRender();
        m_SceneHierarchyPanel->OnUIRender();
        m_PropertyPanel->OnUIRender();
        m_ResourcePanel->OnUIRender();
        m_PerformancePanel->OnUIRender();

        ImGui::End();
        ImGui::ShowDemoWindow();
    }

    void EditorLayer::UI_Toolbar()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        auto& colors = ImGui::GetStyle().Colors;
        const auto& buttonHovered = colors[ImGuiCol_ButtonHovered];
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(buttonHovered.x, buttonHovered.y, buttonHovered.z, 0.5f));
        const auto& buttonActive = colors[ImGuiCol_ButtonActive];
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(buttonActive.x, buttonActive.y, buttonActive.z, 0.5f));

        ImGui::Begin("##toolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        bool toolbarEnabled = (bool)m_Context.scene;

        ImVec4 tintColor = ImVec4(1, 1, 1, 1);
        if (!toolbarEnabled)
            tintColor.w = 0.5f;

        float size = ImGui::GetWindowHeight() - 4.0f;
        ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x * 0.5f) - (size * 0.5f));

        bool hasPlayButton = m_SceneState == SceneState::Edit || m_SceneState == SceneState::Play;
        bool hasSimulateButton = m_SceneState == SceneState::Edit || m_SceneState == SceneState::Simulate;
        bool hasPauseButton = m_SceneState != SceneState::Edit;

        if (hasPlayButton)
        {
            Ref<Texture2D> icon = (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Simulate) ? m_IconPlay : m_IconStop;
            if (ImGui::ImageButton("PlayButton", (ImTextureID)(uint64_t)icon->GetRendererID(), ImVec2(size, size), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0.0f, 0.0f, 0.0f, 0.0f), tintColor) && toolbarEnabled)
            {
                if (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Simulate)
                    OnScenePlay();
                else if (m_SceneState == SceneState::Play)
                    OnSceneStop();
            }
        }

        if (hasSimulateButton)
        {
            if (hasPlayButton)
                ImGui::SameLine();

            Ref<Texture2D> icon = (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Play) ? m_IconSimulate : m_IconStop;
            if (ImGui::ImageButton("SimulateButton", (ImTextureID)(uint64_t)icon->GetRendererID(), ImVec2(size, size), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0.0f, 0.0f, 0.0f, 0.0f), tintColor) && toolbarEnabled)
            {
                if (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Play)
                    OnSceneSimulate();
                else if (m_SceneState == SceneState::Simulate)
                    OnSceneStop();
            }
        }
        if (hasPauseButton)
        {
            bool isPaused = m_Context.scene->IsPaused();
            ImGui::SameLine();
            {
                Ref<Texture2D> icon = m_IconPause;
                if (ImGui::ImageButton("PauseButton", (ImTextureID)(uint64_t)icon->GetRendererID(), ImVec2(size, size), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0.0f, 0.0f, 0.0f, 0.0f), tintColor) && toolbarEnabled)
                {
                    m_Context.scene->SetPaused(!isPaused);
                }
            }

            // Step button
            if (isPaused)
            {
                ImGui::SameLine();
                {
                    Ref<Texture2D> icon = m_IconStep;
                    bool isPaused = m_Context.scene->IsPaused();
                    if (ImGui::ImageButton("StepButton", (ImTextureID)(uint64_t)icon->GetRendererID(), ImVec2(size, size), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0.0f, 0.0f, 0.0f, 0.0f), tintColor) && toolbarEnabled)
                    {
                        m_Context.scene->Step();
                    }
                }
            }
        }
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(3);
        ImGui::End();
    }

    void EditorLayer::OnScenePlay()
    {
        if (m_SceneState == SceneState::Simulate)
            OnSceneStop();

        m_SceneState = SceneState::Play;
        m_Context.scene->OnRuntimeStart();
    }

    void EditorLayer::OnSceneSimulate()
    {
        if (m_SceneState == SceneState::Play)
            OnSceneStop();

        m_SceneState = SceneState::Simulate;
        m_Context.scene->OnSimulationStart();
    }

    void EditorLayer::OnScenePause()
    {
        if (m_SceneState == SceneState::Edit)
            return;

        m_Context.scene->SetPaused(true);
    }

    void EditorLayer::OnSceneStop()
    {
        VKT_CORE_ASSERT(m_SceneState == SceneState::Play || m_SceneState == SceneState::Simulate);

        if (m_SceneState == SceneState::Play)
            m_Context.scene->OnRuntimeStop();
        else if (m_SceneState == SceneState::Simulate)
            m_Context.scene->OnSimulationStop();

        m_SceneState = SceneState::Edit;
    }

    void EditorLayer::OnEvent(Event& event)
    {
        if (m_ViewportPanel->IsFocusedAndHovered())
            m_EditorCamera.OnEvent(event);

        m_ViewportPanel->OnEvent(event);
        m_SceneHierarchyPanel->OnEvent(event);
        m_PropertyPanel->OnEvent(event);

        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<KeyPressedEvent>(VKT_BIND_EVENT_FN(EditorLayer::OnKeyPressed));
        dispatcher.Dispatch<MouseButtonPressedEvent>(VKT_BIND_EVENT_FN(EditorLayer::OnMouseButtonPressed));
    }

    bool EditorLayer::OnKeyPressed(KeyPressedEvent& event)
    {
        bool ctrl = Input::IsKeyPressed(VKT_KEY_LEFT_CONTROL) || Input::IsKeyPressed(VKT_KEY_RIGHT_CONTROL);
        bool shift = Input::IsKeyPressed(VKT_KEY_LEFT_SHIFT) || Input::IsKeyPressed(VKT_KEY_RIGHT_SHIFT);

        if (ctrl && event.GetKeyCode() == VKT_KEY_N) {
            m_Context.signals.Publish<EditorEvents::RequestNewScene>();
            return true;
        }
        else if (ctrl && event.GetKeyCode() == VKT_KEY_O) {
            m_Context.signals.Publish<EditorEvents::RequestOpenScene>();
            return true;
        }
        else if (ctrl && shift && event.GetKeyCode() == VKT_KEY_S) {
            m_Context.signals.Publish<EditorEvents::RequestSaveScene>();
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
        else if (event.GetKeyCode() == VKT_KEY_C) {
            m_Context.isEditorCameraActive = !m_Context.isEditorCameraActive;
            return true;
        }
        return false;
    }

    bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent& event)
    {
        if (m_ViewportPanel->IsFocusedAndHovered() && event.GetMouseButton() == VKT_MOUSE_BUTTON_LEFT)
            return true;
        return false;
    }

    void EditorLayer::NewScene()
    {
        m_Scene = CreateRef<Scene>();
        m_Context.scene = m_Scene;
        m_Context.selectedEntity = Entity{};
        m_Context.hoveredEntity = Entity{};
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
        static FileDialogs fileDialogs;
        std::string filepath = fileDialogs.OpenFile("YAML Files (*.yaml)\0*.yaml\0All Files (*.*)\0*.*\0");
        if (!filepath.empty())
        {
            m_Scene = CreateRef<Scene>();
            SceneSerializer serializer(m_Scene);
            if (serializer.Deserialize(filepath))
            {
                m_Context.scene = m_Scene;
                m_Context.selectedEntity = Entity{};
                m_Context.hoveredEntity = Entity{};
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
        static FileDialogs fileDialogs;
        std::string filepath = fileDialogs.SaveFile("YAML Files (*.yaml)\0*.yaml\0All Files (*.*)\0*.*\0");
        if (!filepath.empty())
        {
            SceneSerializer serializer(m_Scene);
            serializer.Serialize(filepath);
            m_CurrentScenePath = filepath;
        }
    }

    bool EditorLayer::IsViewportFocused() const
    {
        return m_ViewportPanel->IsFocusedAndHovered();
    }

}