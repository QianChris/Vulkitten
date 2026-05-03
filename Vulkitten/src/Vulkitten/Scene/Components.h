#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Renderer/Texture.h"

#include <glm/glm.hpp>

namespace Vulkitten {

    struct TransformComponent
    {
        glm::mat4 Transform{ 1.0f };

        operator glm::mat4() const
        {
            return Transform;
        }
    };

    struct SpriteRendererComponent
    {
        glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
        Ref<Texture2D> Texture{ nullptr };
        float TilingFactor{ 1.0f };
    };

}