#include "vktpch.h"
#include "PreparePass.h"

#include "Vulkitten/Renderer/RenderCommand.h"
#include "Vulkitten/Renderer/Renderer.h"
#include "Vulkitten/Renderer/Framebuffer.h"

namespace Vulkitten {

PreparePass::PreparePass()
{
    name = "PreparePass";

    SetExecute([](const std::vector<RenderGraphResource>& /*resources*/,
                  const std::vector<RenderCommand>& commands,
                  void* /*backendContext*/) {
        auto* graph = Renderer::GetRenderGraph();
        auto framebuffer = graph->GetFramebuffer();

        // Bind custom framebuffer so clear goes to it
        if (framebuffer)
            framebuffer->Bind();

        for (auto& cmd : commands)
        {
            if (auto* clearCmd = std::get_if<ClearCommand>(&cmd))
            {
                if (clearCmd->clearColor)
                    Legacy::RenderCommand::SetClearColor(clearCmd->color);
                Legacy::RenderCommand::Clear();
            }
        }

        if (framebuffer)
            framebuffer->Unbind();
    });
}

} // namespace Vulkitten
