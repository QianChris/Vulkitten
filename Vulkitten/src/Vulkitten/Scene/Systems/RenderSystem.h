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
        const std::string& GetName() const override { return s_Name; }

    private:
        static const std::string s_Name;
        //void RenderQuadComponent();
    };

}