#include "vktpch.h"
#include "EndPass.h"

#include "Vulkitten/Renderer/GraphicsContext.h"

namespace Vulkitten {

EndPass::EndPass()
{
    name = "EndPass";
    writesToSwapchain = true;

    // SwapBuffers has been migrated to IRenderer::EndFrame().
    // EndPass is now a no-op placeholder; can be removed in a future cleanup.
    SetExecute([](const std::vector<RenderGraphResource>& /*resources*/,
                  const std::vector<RenderCommand>& /*commands*/,
                  void* /*backendContext*/) {
        // No-op: SwapBuffers now handled by Renderer::EndFrame()
    });
}

} // namespace Vulkitten
