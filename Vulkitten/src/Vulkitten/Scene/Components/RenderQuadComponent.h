#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Renderer/Texture.h"

#include <glm/glm.hpp>

namespace Vulkitten
{

    struct RenderQuadComponent
    {
        glm::vec4 Color{1.0f};
        Ref<Texture2D> Texture{};
        float TilingFactor{1.0f};

        RenderQuadComponent() = default;
        RenderQuadComponent(const glm::vec4 &color) : Color(color) {}
        RenderQuadComponent(const Ref<Texture2D> &texture) : Texture(texture) {}
        RenderQuadComponent(const glm::vec4 &color, const Ref<Texture2D> &texture, float tilingFactor = 1.0f)
            : Color(color), Texture(texture), TilingFactor(tilingFactor) {}
    };

}