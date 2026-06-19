#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Core/Timestep.h"

namespace Vulkitten {

    class Scene;

    class VKT_API System {
    public:
        virtual ~System() = default;
        virtual bool OnUpdate(Scene& scene, Timestep timestep, bool shouldRender) = 0;
        // Returns the system name for ordering via Scene::SetSystemOrder().
        virtual const std::string& GetName() const = 0;
    };

}