#include "vktpch.h"
#include "SceneHierarchyPanel.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

namespace Vulkitten {

    SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& scene)
    {
        SetContext(scene);
    }

    void SceneHierarchyPanel::SetContext(const Ref<Scene>& scene)
    {
        m_Context = scene;
    }

    void SceneHierarchyPanel::OnImGuiRender()
    {
        ImGui::Begin("Scene Hierarchy");

        if (!m_Context)
        {
            ImGui::Text("No scene loaded");
            ImGui::End();
            return;
        }

        DrawSceneHierarchy();

        ImGui::End();
    }

void SceneHierarchyPanel::DrawSceneHierarchy()
    {
        ImGui::RadioButton("Entity View", &m_SelectedEntityView, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Component View", &m_SelectedEntityView, 1);

        ImGui::Separator();

        if (m_SelectedEntityView == 0)
        {
            DrawEntityView();
        }
        else
        {
            DrawComponentView();
        }
    }

    void SceneHierarchyPanel::DrawEntityView()
    {
        auto& registry = m_Context->GetRegistry();

        std::vector<std::pair<uint32_t, Entity>> entityList;

        auto view = registry.view<entt::entity>();
        for (auto entity : view)
        {
            uint32_t id = static_cast<uint32_t>(entity);
            entityList.push_back({ id, Entity(entity, m_Context.get()) });
        }

        if (entityList.empty())
        {
            ImGui::Text("No entities in scene");
            return;
        }

        for (auto& [id, entity] : entityList)
        {
            std::string label = "Entity " + std::to_string(id);
            bool isSelected = (m_SelectedEntityID == id);

            if (ImGui::TreeNodeEx(label.c_str(), isSelected ? ImGuiTreeNodeFlags_Selected : 0))
            {
                if (ImGui::IsItemClicked())
                {
                    m_SelectedEntityID = id;
                }

                DrawEntityComponents(entity);
                ImGui::TreePop();
            }
        }
    }

    void SceneHierarchyPanel::DrawComponentView()
    {
        ImGui::Text("All Component Types:");
        ImGui::Separator();

        const char* componentTypes[] = {
            "TagComponent",
            "TransformComponent",
            "SpriteRendererComponent",
            "CameraComponent",
            "NativeScriptComponent"
        };

        for (int i = 0; i < IM_ARRAYSIZE(componentTypes); i++)
        {
            if (ImGui::Selectable(componentTypes[i], m_SelectedComponentIndex == i))
            {
                m_SelectedComponentIndex = i;
                m_SelectedComponentType = componentTypes[i];
            }
        }

        if (m_SelectedComponentIndex >= 0)
        {
            ImGui::Separator();
            ImGui::Text("Entities with %s:", componentTypes[m_SelectedComponentIndex]);

            std::vector<std::pair<uint32_t, Entity>> entitiesWithComponent;

            auto& registry = m_Context->GetRegistry();
            auto view = registry.view<entt::entity>();
            for (auto entity : view)
            {
                Entity e(entity, m_Context.get());
                bool hasComponent = false;

                switch (m_SelectedComponentIndex)
                {
                    case 0: hasComponent = e.HasComponent<TagComponent>(); break;
                    case 1: hasComponent = e.HasComponent<TransformComponent>(); break;
                    case 2: hasComponent = e.HasComponent<SpriteRendererComponent>(); break;
                    case 3: hasComponent = e.HasComponent<CameraComponent>(); break;
                    case 4: hasComponent = e.HasComponent<NativeScriptComponent>(); break;
                }

                if (hasComponent)
                {
                    uint32_t id = static_cast<uint32_t>(entity);
                    entitiesWithComponent.push_back({ id, e });
                }
            }

            if (entitiesWithComponent.empty())
            {
                ImGui::Text("No entities have this component");
            }
            else
            {
                DrawComponentEntities(componentTypes[m_SelectedComponentIndex], entitiesWithComponent);
            }
        }
    }

    void SceneHierarchyPanel::DrawEntityComponents(Entity entity)
    {
        if (!entity)
            return;

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
            ImGui::DragFloat3("Position", glm::value_ptr(transform.Position), 0.1f);
            ImGui::DragFloat3("Rotation", glm::value_ptr(transform.Rotation), 1.0f);
            ImGui::DragFloat3("Scale", glm::value_ptr(transform.Scale), 0.1f);
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
            auto& camera = entity.GetComponent<CameraComponent>();
            ImGui::Text("CameraComponent:");
            ImGui::Checkbox("Primary", &camera.Primary);
            ImGui::Checkbox("FixedAspectRatio", &camera.FixedAspectRatio);
        }

        if (entity.HasComponent<NativeScriptComponent>())
        {
            auto& script = entity.GetComponent<NativeScriptComponent>();
            ImGui::Text("NativeScriptComponent:");
            ImGui::Text("ClassName: %s", script.ClassName.c_str());
        }
    }

    void SceneHierarchyPanel::DrawComponentEntities(const char* componentName, std::vector<std::pair<uint32_t, Entity>>& entities)
    {
        for (auto& [id, entity] : entities)
        {
            std::string label = "Entity " + std::to_string(id);
            bool isSelected = (m_SelectedEntityID == id);

            if (ImGui::TreeNodeEx(label.c_str(), isSelected ? ImGuiTreeNodeFlags_Selected : 0))
            {
                if (ImGui::IsItemClicked())
                {
                    m_SelectedEntityID = id;
                }

                DrawEntityComponents(entity);
                ImGui::TreePop();
            }
        }
    }

}