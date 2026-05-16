#pragma once

#include <Vulkitten.h>

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Renderer/EditorCamera.h"
#include "imgui.h"
#include "ImGuizmo.h"

namespace Vulkitten {
    class ViewportPanel
    {
    public:
        ViewportPanel();
        void SetContext(Ref<Scene> scene);
        void SetSelectedEntity(Entity entity) { m_SelectedEntity = entity; }
        void SetEditorCamera(EditorCamera* camera) { m_EditorCamera = camera; }
        void SetOnEntitySelectedCallback(std::function<void(Entity)> callback) { m_OnEntitySelected = callback; }
        void OnImGuiRender();

        bool OnMouseClicked();
		Entity GetSelectedEntity() const { return m_SelectedEntity; }
		Entity GetHoveredEntity() const { return m_HoveredEntity; }

        void UpdateViewportFramebuffer(uint32_t width, uint32_t height);
        Ref<Framebuffer> GetFramebuffer() { return m_Framebuffer; }
        uint32_t GetViewportWidth() const { return m_ViewportWidth; }
        uint32_t GetViewportHeight() const { return m_ViewportHeight; }
        bool IsFocused() const { return m_IsFocused; }
        void SetFocused(bool focused) { m_IsFocused = focused; }
		bool IsFocusedAndHovered() const { return m_IsFocused && m_IsHovered; }

    private:
        void RenderGizmo();
        void QueryScene();

    private:
        Ref<Scene> m_Scene;
        Entity m_SelectedEntity;
        Entity m_HoveredEntity;
        Ref<Framebuffer> m_Framebuffer;
        uint32_t m_ViewportWidth = 1280;
        uint32_t m_ViewportHeight = 720;
        bool m_IsFocused = false;
		bool m_IsHovered = false;
        EditorCamera* m_EditorCamera = nullptr;

        glm::vec2 m_ViewportBounds[2];
        glm::vec2 m_ViewportPos;

        ImGuizmo::OPERATION m_GizmoOperation = ImGuizmo::TRANSLATE;
        ImGuizmo::MODE m_GizmoMode = ImGuizmo::MODE::LOCAL;

        std::function<void(Entity)> m_OnEntitySelected;
    };
}