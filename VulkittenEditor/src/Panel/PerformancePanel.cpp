#include "vktpch.h"
#include "PerformancePanel.h"

#include <imgui.h>

namespace Vulkitten {

    PerformancePanel::PerformancePanel(const Ref<Scene>& scene)
    {
        SetContext(scene);
    }

    void PerformancePanel::SetContext(const Ref<Scene>& scene)
    {
        m_Context = scene;
    }

    void PerformancePanel::OnImGuiRender()
    {
        ImGui::Begin("Performance");

        float fps = Application::Get().GetFPS();
        float frameTime = Application::Get().GetFrameTime();
        ImGui::Text("Actual FPS: %.1f", fps);
        ImGui::Text("Frame Time: %.3f ms ( FPS: %.3f )", frameTime * 1000.0f, 1.0f / frameTime);

        auto& stats = Renderer2D::GetStats();
        ImGui::Separator();
        ImGui::Text("Renderer Stats:");
        ImGui::Text("Draw Calls: %u", stats.DrawCalls);
        ImGui::Text("Quads: %u / %u", stats.Quads, Renderer2D::GetMaxQuads());
        ImGui::Text("Vertices: %u", stats.Vertices);
        ImGui::Text("Texture Count: %u / %u", stats.TextureCount, Renderer2D::GetMaxTextureSlots());

        ImGui::End();
    }

}