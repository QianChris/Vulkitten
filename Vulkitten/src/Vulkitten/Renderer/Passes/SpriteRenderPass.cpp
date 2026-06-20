#include "vktpch.h"
#include "SpriteRenderPass.h"

#include "Vulkitten/Renderer/Renderer2D.h"
#include "Vulkitten/Renderer/RenderContext.h"
#include "Vulkitten/Renderer/Framebuffer.h"

namespace Vulkitten {

SpriteRenderPass::SpriteRenderPass()
{
    name = "SpriteRenderPass";

    SetExecute([](const std::vector<RenderGraphResource>& /*resources*/,
                  const std::vector<RenderCommand>& commands,
                  void* /*backendContext*/) {
        if (commands.empty())
            return;

        auto* graph = RenderContext::Get().GetRenderGraph();
        const auto& perFrame = graph->GetPerFrameData();
        auto* camera = perFrame.Camera;

        if (!camera)
            return;

        auto framebuffer = graph->GetFramebuffer();
        if (framebuffer)
            framebuffer->Bind();

        Renderer2D::BeginScene(*camera);

        for (auto& cmd : commands)
        {
            if (auto* drawCmd = std::get_if<DrawQuadCommand>(&cmd))
            {
                if (drawCmd->texture)
                {
                    Renderer2D::DrawQuad(drawCmd->transform,
                                         drawCmd->texture,
                                         drawCmd->tilingFactor,
                                         drawCmd->color,
                                         0);
                }
                else
                {
                    Renderer2D::DrawQuad(drawCmd->transform,
                                         drawCmd->color,
                                         0);
                }
            }
        }

        Renderer2D::EndScene();

        if (framebuffer)
            framebuffer->Unbind();
    });
}

} // namespace Vulkitten
