#include "vktpch.h"
#include "GpuParticlePass.h"

#include "Vulkitten/Renderer/Renderer.h"
#include "Vulkitten/Scene/Scene.h"
#include "Vulkitten/Scene/Components.h"

namespace Vulkitten {

GpuParticlePass::GpuParticlePass()
{
    name = "GpuParticlePass";

    SetExecute([](const std::vector<RenderGraphResource>& /*resources*/,
                  const std::vector<RenderCommand>& /*commands*/,
                  void* /*backendContext*/) {
        auto* graph = Renderer::GetRenderGraph();
        auto* scene = graph->GetScene();
        auto* camera = graph->GetSceneCamera();

        if (!scene || !camera)
            return;

        // Iterate GPU emitter entities and render them
        auto& registry = scene->GetRegistry();
        auto gpuEmitterView = registry.view<GpuEmitterComponent>();
        for (auto entity : gpuEmitterView)
        {
            auto emitterInstance = scene->GetEmitterManager().GetOrCreateEmitterInstance(
                Entity(entity, const_cast<Scene*>(scene)));
            if (emitterInstance)
            {
                emitterInstance->Update();
                emitterInstance->Render(*camera);
            }
        }
    });
}

} // namespace Vulkitten
