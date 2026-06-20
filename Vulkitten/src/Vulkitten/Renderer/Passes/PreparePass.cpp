#include "vktpch.h"
#include "PreparePass.h"

#include "Vulkitten/Renderer/RenderCommand.h"

namespace Vulkitten {

PreparePass::PreparePass()
{
    name = "PreparePass";

    SetExecute([](const std::vector<RenderGraphResource>& /*resources*/,
                  const std::vector<RenderCommand>& commands,
                  void* /*backendContext*/) {
        for (auto& cmd : commands)
        {
            if (auto* clearCmd = std::get_if<ClearCommand>(&cmd))
            {
                if (clearCmd->clearColor)
                    Legacy::RenderCommand::SetClearColor(clearCmd->color);
                Legacy::RenderCommand::Clear();
            }
        }
    });
}

} // namespace Vulkitten
