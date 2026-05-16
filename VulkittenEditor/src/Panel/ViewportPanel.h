#pragma once
#include "IPanel.h"
#include "Vulkitten/Renderer/Framebuffer.h"
#include "imgui.h"
#include "ImGuizmo.h"

namespace Vulkitten {
    class ViewportPanel : public IPanel {
    public:
        ViewportPanel();
        void OnAttach(EditorContext* context) override;
        void OnUpdate(Timestep ts) override;
        void OnUIRender() override;

        Ref<Framebuffer> GetFramebuffer() { return m_Framebuffer; }
        uint32_t GetViewportWidth() const { return m_ViewportWidth; }
        uint32_t GetViewportHeight() const { return m_ViewportHeight; }
        float GetAspectRatio() const { return m_ViewportHeight > 0 ? (float)m_ViewportWidth / (float)m_ViewportHeight : 1.0f; }
        bool IsFocused() const { return m_IsFocused; }
        bool IsHovered() const { return m_IsHovered; }
        bool IsFocusedAndHovered() const { return m_IsFocused && m_IsHovered; }

    private:
        void RenderGizmo();
        void QueryScene();
        void UpdateViewportFramebuffer(uint32_t width, uint32_t height);

    private:
        Ref<Framebuffer> m_Framebuffer;
        uint32_t m_ViewportWidth = 1280;
        uint32_t m_ViewportHeight = 720;
        bool m_IsFocused = false;
        bool m_IsHovered = false;

        glm::vec2 m_ViewportBounds[2];
        glm::vec2 m_ViewportPos;

        ImGuizmo::OPERATION m_GizmoOperation = ImGuizmo::TRANSLATE;
        ImGuizmo::MODE m_GizmoMode = ImGuizmo::MODE::LOCAL;

        // Gizmo Command 状态
        bool m_GizmoWasUsing = false;
        TransformComponent m_GizmoStartTransform;
    };
} // namespace Vulkitten