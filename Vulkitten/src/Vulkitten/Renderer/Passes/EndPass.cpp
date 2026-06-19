#include "vktpch.h"
#include "EndPass.h"

#include "Vulkitten/Renderer/GraphicsContext.h"

namespace Vulkitten {

EndPass::EndPass()
{
    name = "EndPass";
    writesToSwapchain = true;

    SetExecute([](const std::vector<RenderGraphResource>& /*resources*/,
                  const std::vector<RenderCommand>& /*commands*/,
                  void* backendContext) {
        if (backendContext)
        {
            auto* context = static_cast<GraphicsContext*>(backendContext);
            context->SwapBuffers();
        }
    });
}

} // namespace Vulkitten
