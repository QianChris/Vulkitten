#include "vktpch.h"
#include "PreparePass.h"

#include "Vulkitten/Renderer/IRenderer.h"
#include "Vulkitten/Renderer/Renderer.h"
#include "Vulkitten/Renderer/RenderGraph/RenderGraph.h"
#include "Vulkitten/Renderer/Framebuffer.h"

namespace Vulkitten {

PreparePass::PreparePass()
{
    name = "PreparePass";

    SetExecute([this](const std::vector<RenderGraphResource>& /*resources*/,
                      const std::vector<RenderCommand>& commands,
                      void* /*backendContext*/) {
        auto* api = static_cast<Renderer&>(IRenderer::Get()).GetRendererAPI();

        // Bind the configured Framebuffer (nullptr = default backbuffer)
        auto* graph = GetGraph();
        auto fb = graph ? graph->GetFramebuffer("Viewport") : nullptr;
        if (fb)
            fb->Bind();

        for (auto& cmd : commands)
        {
            if (auto* clearCmd = std::get_if<ClearCommand>(&cmd))
            {
                if (clearCmd->clearColor)
                    api->SetClearColor(clearCmd->color);
                api->Clear();
            }
        }

        if (fb)
            fb->Unbind();
    });
}

} // namespace Vulkitten
