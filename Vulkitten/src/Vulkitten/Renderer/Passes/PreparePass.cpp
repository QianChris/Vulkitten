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
                // Note: depth/stencil clear bits are handled by glClear mask;
                // Legacy::RenderCommand::Clear() does GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT
                Legacy::RenderCommand::Clear();
            }
        }
    });
}

} // namespace Vulkitten
