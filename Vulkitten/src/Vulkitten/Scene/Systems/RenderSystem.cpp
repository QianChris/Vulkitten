#include "vktpch.h"
#include "RenderSystem.h"

namespace Vulkitten {

    bool RenderSystem::OnUpdate(Scene& scene, Timestep timestep, bool shouldRender)
    {
        RenderQuadComponent();
        
        return shouldRender;
    }

    void RenderSystem::RenderQuadComponent()
    {

    }

}