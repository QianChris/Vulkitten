#include "vktpch.h"
#include "PropertyPanel.h"

#include "Vulkitten/Scene/SceneCamera.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

namespace Vulkitten {

    PropertyPanel::PropertyPanel(const Ref<Scene>& scene)
    {
        SetContext(scene);
    }

    void PropertyPanel::SetContext(const Ref<Scene>& scene)
    {
        m_Context = scene;
    }

    void PropertyPanel::SetSelectedEntity(Entity entity)
    {
        m_SelectedEntity = entity;
    }

    void PropertyPanel::OnImGuiRender()
    {
        ImGui::Begin("Properties");

        if (!m_Context)
        {
            ImGui::Text("No scene loaded");
            ImGui::End();
            return;
        }

        if (!m_SelectedEntity)
        {
            ImGui::Text("No entity selected");
            ImGui::End();
            return;
        }

        DrawEntityComponents(m_SelectedEntity);

        ImGui::End();
    }

    void PropertyPanel::DrawEntityComponents(Entity entity)
    {
        if (!entity)
            return;

        uint32_t entityID = entity.GetEntityID();
        ImGui::Text("Entity ID: %u", entityID);
        ImGui::Separator();

        if (entity.HasComponent<TagComponent>())
        {
            auto& tag = entity.GetComponent<TagComponent>();
            ImGui::Text("TagComponent:");
            static char tagBuffer[256] = {};
            strcpy_s(tagBuffer, tag.Tag.c_str());
            if (ImGui::InputText("Tag", tagBuffer, IM_ARRAYSIZE(tagBuffer)))
            {
                tag.Tag = tagBuffer;
            }
        }

        if (entity.HasComponent<TransformComponent>())
        {
            auto& transform = entity.GetComponent<TransformComponent>();
            ImGui::Text("TransformComponent:");

            glm::vec3 position = transform.Position;
            glm::vec3 rotation = transform.Rotation;
            glm::vec3 scale = transform.Scale;

            if (ImGui::DragFloat3("Position", glm::value_ptr(position), 0.1f))
            {
                transform.SetPosition(position);
            }
            if (ImGui::DragFloat3("Rotation", glm::value_ptr(rotation), 1.0f))
            {
                transform.SetRotation(rotation);
            }
            if (ImGui::DragFloat3("Scale", glm::value_ptr(scale), 0.1f))
            {
                transform.SetScale(scale);
            }
        }

        if (entity.HasComponent<SpriteRendererComponent>())
        {
            auto& sprite = entity.GetComponent<SpriteRendererComponent>();
            ImGui::Text("SpriteRendererComponent:");
            ImGui::ColorEdit4("Color", glm::value_ptr(sprite.Color));
            ImGui::DragFloat("TilingFactor", &sprite.TilingFactor, 0.1f);
        }

        if (entity.HasComponent<CameraComponent>())
        {
            auto& cameraComponent = entity.GetComponent<CameraComponent>();
            auto& camera = cameraComponent.Camera;
            ImGui::Text("CameraComponent:");

            ImGui::Checkbox("Primary", &cameraComponent.Primary);
            ImGui::Checkbox("FixedAspectRatio", &cameraComponent.FixedAspectRatio);

            const char* projectionTypes[] = { "Perspective", "Orthographic" };
            int currentProjection = static_cast<int>(camera.GetProjectionType());
            if (ImGui::Combo("Projection Type", &currentProjection, projectionTypes, IM_ARRAYSIZE(projectionTypes)))
            {
                camera.SetProjectionType(static_cast<SceneCamera::ProjectionType>(currentProjection));
            }

            if (camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
            {
                float fov = glm::degrees(camera.GetPerspectiveFOV());
                if (ImGui::DragFloat("FOV", &fov, 1.0f, 1.0f, 180.0f))
                {
                    camera.SetPerspectiveFOV(glm::radians(fov));
                }
                float nearClip = camera.GetPerspectiveNear();
                if (ImGui::DragFloat("Near", &nearClip, 0.01f, 0.001f, 100.0f))
                {
                    camera.SetPerspectiveNear(nearClip);
                }
                float farClip = camera.GetPerspectiveFar();
                if (ImGui::DragFloat("Far", &farClip, 1.0f, 0.1f, 2000.0f))
                {
                    camera.SetPerspectiveFar(farClip);
                }
            }
            else
            {
                float size = camera.GetOrthographicSize();
                if (ImGui::DragFloat("Size", &size, 0.1f, 0.1f, 100.0f))
                {
                    camera.SetOrthographicSize(size);
                }
                float nearClip = camera.GetOrthographicNear();
                if (ImGui::DragFloat("Near", &nearClip, 0.1f, -100.0f, 100.0f))
                {
                    camera.SetOrthographicNear(nearClip);
                }
                float farClip = camera.GetOrthographicFar();
                if (ImGui::DragFloat("Far", &farClip, 0.1f, -100.0f, 100.0f))
                {
                    camera.SetOrthographicFar(farClip);
                }
            }
        }

        if (entity.HasComponent<NativeScriptComponent>())
        {
            auto& script = entity.GetComponent<NativeScriptComponent>();
            ImGui::Text("NativeScriptComponent:");
            ImGui::Text("ClassName: %s", script.ClassName.c_str());
        }
    }

}