#pragma once

#include "Vulkitten/Core.h"

#include "Vulkitten/Renderer/RendererAPI.h"
#include "Vulkitten/Renderer/VertexArray.h"

#include <glm/glm.hpp>

namespace Vulkitten {

    class VKT_API RenderCommand
    {
    public:
        inline static void Init()
        {
            s_RendererAPI->Init();
        }
        inline static void SetClearColor(const glm::vec4& color)
        {
            s_RendererAPI->SetClearColor(color);
        };
        inline static void Clear()
        {
            s_RendererAPI->Clear();
        };

        inline static void DrawIndexed(const Ref<VertexArray>& vertexArray)
        {
            s_RendererAPI->DrawIndexed(vertexArray);
        };
    private:
        static RendererAPI* s_RendererAPI;
    };

}