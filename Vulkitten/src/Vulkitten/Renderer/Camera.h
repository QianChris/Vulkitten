#pragma once

#include "Vulkitten/Core/Core.h"

#include <glm/glm.hpp>

namespace Vulkitten {

    class VKT_API Camera
    {
    public:
        Camera() = default;
        virtual ~Camera() = default;

        const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }

    protected:
        glm::mat4 m_ProjectionMatrix{ 1.0f };
    };

}