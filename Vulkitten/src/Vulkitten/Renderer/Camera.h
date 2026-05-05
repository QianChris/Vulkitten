#pragma once

#include "Vulkitten/Core/Core.h"

#include <glm/glm.hpp>

namespace Vulkitten {

class VKT_API Camera
    {
    public:
        Camera() = default;
        virtual ~Camera() = default;

        virtual const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
        virtual const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
        virtual const glm::mat4& GetViewProjectionMatrix() const { return m_ProjectionMatrix * m_ViewMatrix; }

    protected:
        glm::mat4 m_ProjectionMatrix{ 1.0f };
        glm::mat4 m_ViewMatrix{ 1.0f };
    };

}