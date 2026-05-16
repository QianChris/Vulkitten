#include "SceneHierarchyPanel.h"
#include "EditorCommand.h"
#include "imgui.h"

namespace Vulkitten {

    void SceneHierarchyPanel::OnAttach(EditorContext* context)
    {
        IPanel::OnAttach(context);

        // 同步外部选中信号到本地树高亮
        context->signals.Subscribe<EditorEvents::EntitySelected>(
            [this](const auto& evt) {
                m_SelectedEntityID = evt.entity ? evt.entity.GetEntityID() : ~0u;
            });
    }

    void SceneHierarchyPanel::OnUIRender()
    {
        ImGui::Begin("Scene Hierarchy", &IsOpen);

        if (!m_Context || !m_Context->scene)
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
        auto scene = m_Context->scene;

        if (ImGui::Button("Add Entity"))
        {
            scene->CreateEntity("New Entity");
            m_Context->signals.Publish<EditorEvents::SceneModified>();
        }
        ImGui::SameLine();

        bool hasSelection = m_SelectedEntityID != ~0u;
        ImGui::BeginDisabled(!hasSelection);
        if (ImGui::Button("Delete Entity"))
        {
            if (hasSelection)
            {
                Entity entity((entt::entity)m_SelectedEntityID, scene.get());
                if (entity)
                {
                    if (m_Context->commands)
                        m_Context->commands->Execute(CreateRef<DestroyEntityCommand>(scene, entity));
                    else
                        scene->DestroyEntity(entity);

                    if (m_Context->selectedEntity == entity)
                        m_Context->SelectEntity(Entity{});

                    m_Context->signals.Publish<EditorEvents::EntityDestroyed>(m_SelectedEntityID);
                }
            }
        }
        ImGui::EndDisabled();

        auto& registry = scene->GetRegistry();
        std::vector<std::pair<uint32_t, Entity>> entityList;
        auto view = registry.view<entt::entity>();
        for (auto entity : view)
        {
            uint32_t id = static_cast<uint32_t>(entity);
            entityList.push_back({ id, Entity(entity, scene.get()) });
        }

        if (entityList.empty())
        {
            ImGui::Text("No entities in scene");
            return;
        }

        Entity hoveredEntity;
        for (auto& [id, entity] : entityList)
        {
            std::string label = entity.HasComponent<TagComponent>()
                ? entity.GetComponent<TagComponent>().Tag
                : "Entity " + std::to_string(id);
            bool isSelected = (m_SelectedEntityID == id);

            if (ImGui::Selectable(label.c_str(), isSelected))
            {
                m_Context->SelectEntity(entity);  // 发信号，不直接改 ID
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly))
                hoveredEntity = entity;
        }

        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0) && !hoveredEntity)
            m_Context->SelectEntity(Entity{});

        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(1))
        {
            if (hoveredEntity) ImGui::OpenPopup("EntityContextMenu");
            else               ImGui::OpenPopup("EmptySpaceContextMenu");
        }

        if (ImGui::BeginPopup("EntityContextMenu"))
        {
            if (hoveredEntity && ImGui::MenuItem("Delete Entity"))
            {
                if (m_Context->commands)
                    m_Context->commands->Execute(CreateRef<DestroyEntityCommand>(scene, hoveredEntity));
                else
                    scene->DestroyEntity(hoveredEntity);

                if (m_Context->selectedEntity == hoveredEntity)
                    m_Context->SelectEntity(Entity{});

                m_Context->signals.Publish<EditorEvents::EntityDestroyed>(hoveredEntity.GetEntityID());
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup("EmptySpaceContextMenu"))
        {
            if (ImGui::MenuItem("Create Entity"))
            {
                scene->CreateEntity("New Entity");
                m_Context->signals.Publish<EditorEvents::SceneModified>();
            }
            ImGui::EndPopup();
        }
    }

    void SceneHierarchyPanel::DrawComponentView()
    {
        auto scene = m_Context->scene;
        auto& registry = scene->GetRegistry();
        auto view = registry.view<entt::entity>();

        const char* componentTypes[] = {
            "TagComponent", "TransformComponent", "SpriteRendererComponent",
            "CameraComponent", "NativeScriptComponent"
        };

        for (int i = 0; i < IM_ARRAYSIZE(componentTypes); i++)
        {
            std::vector<std::pair<uint32_t, Entity>> entitiesWithComponent;
            for (auto entity : view)
            {
                Entity e(entity, scene.get());
                bool has = false;
                switch (i)
                {
                    case 0: has = e.HasComponent<TagComponent>(); break;
                    case 1: has = e.HasComponent<TransformComponent>(); break;
                    case 2: has = e.HasComponent<SpriteRendererComponent>(); break;
                    case 3: has = e.HasComponent<CameraComponent>(); break;
                    case 4: has = e.HasComponent<NativeScriptComponent>(); break;
                }
                if (has) entitiesWithComponent.push_back({ (uint32_t)entity, e });
            }

            if (!entitiesWithComponent.empty())
            {
                std::string label = std::string(componentTypes[i]) + " (" + std::to_string(entitiesWithComponent.size()) + ")";
                if (ImGui::TreeNodeEx(label.c_str(), 0))
                {
                    for (auto& [id, entity] : entitiesWithComponent)
                    {
                        std::string name = entity.HasComponent<TagComponent>()
                            ? entity.GetComponent<TagComponent>().Tag
                            : "Entity " + std::to_string(id);
                        if (ImGui::Selectable(name.c_str(), m_SelectedEntityID == id))
                            m_Context->SelectEntity(entity);
                    }
                    ImGui::TreePop();
                }
            }
        }
    }

} // namespace Vulkitten