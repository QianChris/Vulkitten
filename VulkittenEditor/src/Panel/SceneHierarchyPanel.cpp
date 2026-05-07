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

    Entity SceneHierarchyPanel::GetSelectedEntity() const
    {
        if (m_SelectedEntityID == INVALID_SELECT)
            return Entity();

        return Entity(static_cast<entt::entity>(m_SelectedEntityID), m_Context.get());
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
        if (ImGui::BeginTabBar("SceneHierarchyTabs"))
        {
            if (ImGui::BeginTabItem("Entity View"))
            {
                DrawEntityView();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Component View"))
            {
                DrawComponentView();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
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

        bool selectionChanged = false;
        for (auto& [id, entity] : entityList)
        {
            std::string label = entity.HasComponent<TagComponent>() 
                ? entity.GetComponent<TagComponent>().Tag 
                : "Entity " + std::to_string(id);
            
            bool isSelected = (m_SelectedEntityID == id);

            if (ImGui::Selectable(label.c_str(), isSelected))
            {
                m_SelectedEntityID = id;
                selectionChanged = true;
            }
        }

        if (!selectionChanged && ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0))
        {
            m_SelectedEntityID = INVALID_SELECT;
        }
    }

    void SceneHierarchyPanel::DrawComponentView()
    {
        const char* componentTypes[] = {
            "TagComponent",
            "TransformComponent",
            "SpriteRendererComponent",
            "CameraComponent",
            "NativeScriptComponent"
        };

        auto& registry = m_Context->GetRegistry();
        auto view = registry.view<entt::entity>();

        for (int i = 0; i < IM_ARRAYSIZE(componentTypes); i++)
        {
            std::vector<std::pair<uint32_t, Entity>> entitiesWithComponent;

            for (auto entity : view)
            {
                Entity e(entity, m_Context.get());
                bool hasComponent = false;

                switch (i)
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

            if (!entitiesWithComponent.empty())
            {
                std::string treeLabel = std::string(componentTypes[i]) + " (" + std::to_string(entitiesWithComponent.size()) + ")";
                if (ImGui::TreeNodeEx(treeLabel.c_str(), 0))
                {
                    for (auto& [id, entity] : entitiesWithComponent)
                    {
                        std::string label = entity.HasComponent<TagComponent>() 
                            ? entity.GetComponent<TagComponent>().Tag 
                            : "Entity " + std::to_string(id);
                        
                        bool isSelected = (m_SelectedEntityID == id);

                        if (ImGui::Selectable(label.c_str(), isSelected))
                        {
                            m_SelectedEntityID = id;
                        }
                    }
                    ImGui::TreePop();
                }
            }
        }
    }

    }