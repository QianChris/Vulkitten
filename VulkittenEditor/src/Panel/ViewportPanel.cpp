#include "ViewportPanel.h"
#include "EditorCommand.h"
#include "imgui.h"
#include <glm/gtc/type_ptr.hpp>

namespace Vulkitten {

    ViewportPanel::ViewportPanel()
    {
        FramebufferSpecification fbSpec;
        fbSpec.Attachments = {
            { FramebufferTextureFormat::RGBA8 },
            { FramebufferTextureFormat::RED_INTEGER },
            { FramebufferTextureFormat::DEPTH }
        };
        fbSpec.Width = 1280;
        fbSpec.Height = 960;
        m_Framebuffer = Framebuffer::Create(fbSpec);
    }

    void ViewportPanel::OnAttach(EditorContext* context)
    {
        IPanel::OnAttach(context);
    }

    void ViewportPanel::UpdateViewportFramebuffer(uint32_t width, uint32_t height)
    {
        if (m_ViewportWidth != width || m_ViewportHeight != height)
        {
            m_ViewportWidth = width;
            m_ViewportHeight = height;
            m_Framebuffer->Resize(width, height);
            if (m_Context)
                m_Context->signals.Publish<EditorEvents::ViewportResized>(width, height);
        }
    }

    void ViewportPanel::OnUpdate(Timestep ts)
    {
        // 相机更新在 EditorLayer 处理，这里保留给后续扩展（如动画预览）
    }

    void ViewportPanel::OnUIRender()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Viewport", &IsOpen);

        auto viewportPos = ImGui::GetWindowPos();
        auto regionMin = ImGui::GetWindowContentRegionMin();
        auto regionMax = ImGui::GetWindowContentRegionMax();
        m_ViewportPos = { viewportPos.x, viewportPos.y };
        m_ViewportBounds[0] = { regionMin.x, regionMin.y };
        m_ViewportBounds[1] = { regionMax.x, regionMax.y };

        m_IsFocused = ImGui::IsWindowFocused();
        m_IsHovered = ImGui::IsWindowHovered();

        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        UpdateViewportFramebuffer((uint32_t)viewportPanelSize.x, (uint32_t)viewportPanelSize.y);

        if (m_Framebuffer)
        {
            uint32_t textureID = m_Framebuffer->GetColorAttachmentRendererID();
            ImGui::Image((void*)(intptr_t)textureID, viewportPanelSize, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));

            // 鼠标拾取
            if (ImGui::IsItemHovered() && m_Context)
            {
                QueryScene();
                if (ImGui::IsItemClicked())
                {
                    if (m_Context->hoveredEntity)
                        m_Context->SelectEntity(m_Context->hoveredEntity);
                    else
                        m_Context->SelectEntity(Entity{});
                }
            }

            RenderGizmo();
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void ViewportPanel::QueryScene()
    {
        if (!m_Context || !m_Context->scene || !m_Framebuffer)
            return;

        auto [mouseX, mouseY] = ImGui::GetMousePos();
        float x = mouseX - m_ViewportBounds[0].x - m_ViewportPos.x;
        float y = mouseY - m_ViewportBounds[0].y - m_ViewportPos.y;
        glm::vec2 viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];

        int pixelX = (int)x;
        int pixelY = (int)(viewportSize.y - y);

        if (pixelX >= 0 && pixelY >= 0 && pixelX < (int)m_ViewportWidth && pixelY < (int)m_ViewportHeight)
        {
            int entityID = m_Framebuffer->ReadPixel(1, pixelX, pixelY);
            if (entityID > 0)
                m_Context->SetHoveredEntity(m_Context->scene->GetEntityByID((uint32_t)entityID));
            else
                m_Context->SetHoveredEntity(Entity{});
        }
    }

    void ViewportPanel::RenderGizmo()
    {
        if (!m_Context || !m_Context->selectedEntity || !m_IsFocused)
            return;

        Entity selected = m_Context->selectedEntity;
        if (!selected.HasComponent<TransformComponent>())
            return;

        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();

        ImVec2 viewportPos = ImGui::GetWindowPos();
        float windowWidth = ImGui::GetWindowWidth();
        float windowHeight = ImGui::GetWindowHeight();
        ImGuizmo::SetRect(viewportPos.x, viewportPos.y, windowWidth, windowHeight);

        glm::mat4 viewMatrix;
        glm::mat4 projectionMatrix;

        if (m_Context->editorCamera && m_IsFocused)
        {
            viewMatrix = m_Context->editorCamera->GetViewMatrix();
            projectionMatrix = m_Context->editorCamera->GetProjectionMatrix();
        }
        else
        {
            auto& cameraEntity = m_Context->scene->GetPrimaryCameraEntity();
            if (cameraEntity)
            {
                auto& cameraComp = cameraEntity.GetComponent<CameraComponent>();
                auto& cameraTransform = cameraEntity.GetComponent<TransformComponent>();
                viewMatrix = glm::inverse(cameraTransform.GetTransform());
                projectionMatrix = cameraComp.Camera.GetProjectionMatrix();
            }
            else
            {
                return;
            }
        }

        auto& transformComp = selected.GetComponent<TransformComponent>();
        glm::mat4 modelMatrix = transformComp.GetTransform();

        // 记录 Gizmo 开始状态
        bool isUsing = ImGuizmo::IsUsing();
        if (isUsing && !m_GizmoWasUsing)
        {
            m_GizmoStartTransform = transformComp;
            m_GizmoWasUsing = true;
        }

        ImGuizmo::Manipulate(
            glm::value_ptr(viewMatrix),
            glm::value_ptr(projectionMatrix),
            m_GizmoOperation,
            m_GizmoMode,
            glm::value_ptr(modelMatrix)
        );

        if (ImGuizmo::IsUsing())
        {
            glm::vec3 translation, rotation, scale;
            ImGuizmo::DecomposeMatrixToComponents(
                glm::value_ptr(modelMatrix),
                glm::value_ptr(translation),
                glm::value_ptr(rotation),
                glm::value_ptr(scale)
            );
            transformComp.SetPosition(translation);
            transformComp.SetRotation(rotation);
            transformComp.SetScale(scale);
        }

        // Gizmo 释放：提交 Command
        if (!ImGuizmo::IsUsing() && m_GizmoWasUsing)
        {
            m_GizmoWasUsing = false;
            if (m_Context && m_Context->commands)
            {
                m_Context->commands->Execute(CreateRef<SetTransformCommand>(
                    selected, m_GizmoStartTransform, transformComp));
                m_Context->signals.Publish<EditorEvents::ComponentModified>(selected, "TransformComponent");
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_T))
            m_GizmoOperation = ImGuizmo::TRANSLATE;
        if (ImGui::IsKeyPressed(ImGuiKey_R))
            m_GizmoOperation = ImGuizmo::ROTATE;
        if (ImGui::IsKeyPressed(ImGuiKey_Y))
            m_GizmoOperation = ImGuizmo::SCALE;
    }

} // namespace Vulkitten