#pragma once

#include "System.h"

namespace Vulkitten {

    class Scene;

    class VKT_API RenderSystem : public System
    {
    public:
        RenderSystem() = default;
        virtual ~RenderSystem() = default;

        bool OnUpdate(Scene& scene, Timestep timestep, bool shouldRender) override;
    
    private:
        //void RenderQuadComponent();
    };

}