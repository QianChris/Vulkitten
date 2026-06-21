#include "vktpch.h"
#include "PreparePass.h"

#include "Vulkitten/Renderer/IRenderer.h"
#include "Vulkitten/Renderer/Backend/OpenGL/OpenGLRenderer.h"
#include "Vulkitten/Renderer/RenderGraph/RenderGraph.h"
#include "Vulkitten/Renderer/Framebuffer.h"

namespace Vulkitten {

PreparePass::PreparePass()
{
    name = "PreparePass";

    SetExecute([this](const std::vector<RenderGraphResource>& /*resources*/,
                      const std::vector<RenderCommand>& commands,
                      void* /*backendContext*/) {
        // [HACK: 过渡期仍使用 OpenGLRendererAPI — Task 14 迁移到 ICommandBuffer::BeginRenderPass]
        auto* api = static_cast<OpenGLRenderer&>(IRenderer::Get()).GetRendererAPI();

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
