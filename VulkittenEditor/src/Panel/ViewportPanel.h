#pragma once

#include <Vulkitten.h>

#include "Vulkitten/Core/Core.h"
#include "imgui.h"
#include "ImGuizmo.h"

namespace Vulkitten {
    class ViewportPanel
    {
    public:
        ViewportPanel();
        void SetContext(Ref<Scene> scene);
        void SetSelectedEntity(Entity entity);
        void OnImGuiRender();

        void UpdateViewportFramebuffer(uint32_t width, uint32_t height);
        Ref<FrameBuffer> GetFrameBuffer() { return m_Framebuffer; }
        uint32_t GetViewportWidth() const { return m_ViewportWidth; }
        uint32_t GetViewportHeight() const { return m_ViewportHeight; }

    private:
        void RenderGizmo();

    private:
        Ref<Scene> m_Scene;
        Entity m_SelectedEntity;
        Ref<FrameBuffer> m_Framebuffer;
        uint32_t m_ViewportWidth = 1280;
        uint32_t m_ViewportHeight = 720;

        ImGuizmo::OPERATION m_GizmoOperation = ImGuizmo::TRANSLATE;
        ImGuizmo::MODE m_GizmoMode = ImGuizmo::MODE::LOCAL;
    };
}