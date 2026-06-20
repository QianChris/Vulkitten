#include "vktpch.h"
#include "PreparePass.h"

#include "Vulkitten/Renderer/RenderContext.h"

namespace Vulkitten {

PreparePass::PreparePass()
{
    name = "PreparePass";

    SetExecute([](const std::vector<RenderGraphResource>& /*resources*/,
                  const std::vector<RenderCommand>& commands,
                  void* /*backendContext*/) {
        auto* api = RenderContext::Get().GetRenderer().GetRendererAPI();
        for (auto& cmd : commands)
        {
            if (auto* clearCmd = std::get_if<ClearCommand>(&cmd))
            {
                if (clearCmd->clearColor)
                    api->SetClearColor(clearCmd->color);
                api->Clear();
            }
        }
    });
}

} // namespace Vulkitten
