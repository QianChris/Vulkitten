#pragma once

#include "Vulkitten/Core/Core.h"

#include <glm/glm.hpp>

namespace Vulkitten {

class VKT_API Camera
    {
    public:
        Camera() = default;
        Camera(const glm::mat4& projection)
            : m_ProjectionMatrix(projection) {}
        virtual ~Camera() = default;

        virtual const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
        virtual glm::mat4 GetViewProjectionMatrix() const = 0;

    protected:
        glm::mat4 m_ProjectionMatrix{ 1.0f };
    };

}