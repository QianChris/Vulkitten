#include "vktpch.h"
#include "ViewportPanel.h"

#include "imgui.h"
#include <glm/gtc/type_ptr.hpp>

namespace Vulkitten {
    ViewportPanel::ViewportPanel()
    {
        Vulkitten::FramebufferSpecification fbSpec;
        fbSpec.Attachments = {
            { FramebufferTextureFormat::RGBA8 },
            { FramebufferTextureFormat::RED_INTEGER },
            { FramebufferTextureFormat::DEPTH }
        };
        fbSpec.Width = 1280;
        fbSpec.Height = 960;
        m_Framebuffer = Vulkitten::Framebuffer::Create(fbSpec);
    }

    void ViewportPanel::SetContext(Vulkitten::Ref<Vulkitten::Scene> scene)
    {
        m_Scene = scene;
    }

    void ViewportPanel::SetSelectedEntity(Vulkitten::Entity entity)
    {
        m_SelectedEntity = entity;
    }

void ViewportPanel::UpdateViewportFramebuffer(uint32_t width, uint32_t height)
    {
        if (m_ViewportWidth != width || m_ViewportHeight != height)
        {
            m_ViewportWidth = width;
            m_ViewportHeight = height;

            m_Framebuffer->Resize(width, height);
        }
    }

    void ViewportPanel::OnImGuiRender()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Viewport");

        auto viewportPos = ImGui::GetWindowPos();
        auto regionMin = ImGui::GetWindowContentRegionMin();
        auto regionMax = ImGui::GetWindowContentRegionMax();
        m_ViewportPos = { viewportPos.x, viewportPos.y };
        m_ViewportBounds[0] = { regionMin.x, regionMin.y };
        m_ViewportBounds[1] = { regionMax.x, regionMax.y };

        ImGui::PopStyleVar();

        m_IsFocused = ImGui::IsWindowFocused();
		m_IsHovered = ImGui::IsWindowHovered();

        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        UpdateViewportFramebuffer((uint32_t)viewportPanelSize.x, (uint32_t)viewportPanelSize.y);

        if (m_Framebuffer)
        {
            uint32_t textureID = m_Framebuffer->GetColorAttachmentRendererID();
            ImGui::Image((void*)(intptr_t)textureID, viewportPanelSize, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
            RenderGizmo();
        }

        ImGui::End();
    }

    void ViewportPanel::RenderGizmo()
    {
        if (m_SelectedEntity && ImGui::IsWindowFocused())
        {
            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist();

            ImGuiIO& io = ImGui::GetIO();
            ImVec2 viewportPos = ImGui::GetWindowPos();
            ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
            float windowWidth = ImGui::GetWindowWidth();
            float windowHeight = ImGui::GetWindowHeight();
            ImGuizmo::SetRect(viewportPos.x, viewportPos.y, windowWidth, windowHeight);

            glm::mat4 viewMatrix;
            glm::mat4 projectionMatrix;

            if (m_EditorCamera && m_IsFocused)
            {
                viewMatrix = m_EditorCamera->GetViewMatrix();
                projectionMatrix = m_EditorCamera->GetProjectionMatrix();
            }
            else
            {
                auto& cameraEntity = m_Scene->GetPrimaryCameraEntity();
                if (cameraEntity)
                {
                    auto& cameraComp = cameraEntity.GetComponent<Vulkitten::CameraComponent>();
                    auto& cameraTransform = cameraEntity.GetComponent<Vulkitten::TransformComponent>();

                    viewMatrix = glm::inverse(cameraTransform.GetTransform());
                    projectionMatrix = cameraComp.Camera.GetProjectionMatrix();
                }
            }

            if (m_EditorCamera && m_IsFocused || m_Scene->GetPrimaryCameraEntity())
            {
                auto& transformComp = m_SelectedEntity.GetComponent<Vulkitten::TransformComponent>();
                glm::mat4 modelMatrix = transformComp.GetTransform();

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
            }

if (ImGui::IsKeyPressed(ImGuiKey_T))
                m_GizmoOperation = ImGuizmo::TRANSLATE;
            if (ImGui::IsKeyPressed(ImGuiKey_R))
                m_GizmoOperation = ImGuizmo::ROTATE;
            if (ImGui::IsKeyPressed(ImGuiKey_Y))
                m_GizmoOperation = ImGuizmo::SCALE;
        }
    }

    bool ViewportPanel::QueryScene() {
        if (!m_Scene || !m_Framebuffer)
            return false;

        auto [mouseX, mouseY] = ImGui::GetMousePos();

        float x = mouseX - m_ViewportBounds[0].x - m_ViewportPos.x;
        float y = mouseY - m_ViewportBounds[0].y - m_ViewportPos.y;
        glm::vec2 viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];

        int pixelX = (int)x;
        int pixelY = (int)(viewportSize.y - y);

        int entityID = m_Framebuffer->ReadPixel(1, pixelX, pixelY);

        VKT_CORE_INFO("click pos: x {}, y {}, entity id: {}", pixelX, pixelY, entityID);
        if (entityID > 0)
        {
            m_SelectedEntity = m_Scene->GetEntityByID((uint32_t)entityID);
            return true;
        }
        return false;
    }

    bool ViewportPanel::OnMouseClicked()
    {
        if (!m_Scene || !m_Framebuffer)
            return false;

        auto [mouseX, mouseY] = ImGui::GetMousePos();

        float x = mouseX - m_ViewportBounds[0].x - m_ViewportPos.x;
        float y = mouseY - m_ViewportBounds[0].y - m_ViewportPos.y;
        glm::vec2 viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];

        int pixelX = (int)x;
        int pixelY = (int)(viewportSize.y - y);

        int entityID = m_Framebuffer->ReadPixel(1, pixelX, pixelY);

        VKT_CORE_INFO("click pos: x {}, y {}, entity id: {}", pixelX, pixelY, entityID);
        if (entityID > 0)
        {
            m_SelectedEntity = m_Scene->GetEntityByID((uint32_t)entityID);
            return true;
        }
        return false;
    }
}