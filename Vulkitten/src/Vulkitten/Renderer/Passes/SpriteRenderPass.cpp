#include "vktpch.h"
#include "SpriteRenderPass.h"

#include "Vulkitten/Renderer/Renderer2D.h"
#include "Vulkitten/Renderer/OrthographicCamera.h"
#include "Vulkitten/Renderer/Renderer.h"

namespace Vulkitten {

SpriteRenderPass::SpriteRenderPass()
{
    name = "SpriteRenderPass";

    SetExecute([](const std::vector<RenderGraphResource>& /*resources*/,
                  const std::vector<RenderCommand>& commands,
                  void* /*backendContext*/) {
        if (commands.empty())
            return;

        auto* graph = Renderer::GetRenderGraph();
        auto* camera = graph->GetSceneCamera();

        if (!camera)
            return;

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
    });
}

} // namespace Vulkitten
