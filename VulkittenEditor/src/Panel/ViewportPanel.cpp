#include "vktpch.h"
#include "ViewportPanel.h"

#include "imgui.h"
#include <glm/gtc/type_ptr.hpp>

namespace Vulkitten {
    ViewportPanel::ViewportPanel()
    {
        Vulkitten::FrameBufferSpecification fbSpec;
        fbSpec.Width = 1280;
        fbSpec.Height = 960;
        m_Framebuffer = Vulkitten::FrameBuffer::Create(fbSpec);
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

            //Vulkitten::FrameBufferSpecification fbSpec;
            //fbSpec.Width = width;
            //fbSpec.Height = height;
            //m_Framebuffer = Vulkitten::FrameBuffer::Create(fbSpec);
        }
    }

    void ViewportPanel::OnImGuiRender()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Viewport");
        ImGui::PopStyleVar();

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

            auto& cameraEntity = m_Scene->GetPrimaryCameraEntity();
            if (cameraEntity)
            {
                auto& cameraComp = cameraEntity.GetComponent<Vulkitten::CameraComponent>();
                auto& cameraTransform = cameraEntity.GetComponent<Vulkitten::TransformComponent>();

                glm::mat4 viewMatrix = glm::inverse(cameraTransform.GetTransform());
                glm::mat4 projectionMatrix = cameraComp.Camera.GetProjectionMatrix();

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
}