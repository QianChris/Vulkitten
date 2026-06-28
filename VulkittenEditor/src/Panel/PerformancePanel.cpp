#include "PerformancePanel.h"
#include "imgui.h"
#include "Vulkitten/Core/Application.h"

namespace Vulkitten {

    void PerformancePanel::OnAttach(EditorContext* context)
    {
        IPanel::OnAttach(context);
    }

    void PerformancePanel::OnUIRender()
    {
        ImGui::Begin("Performance", &IsOpen);

        float fps = Application::Get().GetFPS();
        float frameTime = Application::Get().GetFrameTime();
        ImGui::Text("Actual FPS: %.1f", fps);
        ImGui::Text("Frame Time: %.3f ms ( FPS: %.3f )", frameTime * 1000.0f, 1.0f / frameTime);

        // Renderer2D stats removed - SpriteRenderPass now owns batch state
        // TODO: expose batch stats from SpriteRenderPass

        ImGui::End();
    }

} // namespace Vulkitten