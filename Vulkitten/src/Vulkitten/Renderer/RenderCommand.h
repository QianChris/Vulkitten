#pragma once

#include "Vulkitten/Core/Core.h"

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
        inline static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
        {
            s_RendererAPI->SetViewport(x, y, width, height);
        };
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

        inline static void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t count)
        {
            s_RendererAPI->DrawIndexed(vertexArray, count);
        };
    private:
        static RendererAPI* s_RendererAPI;
    };

}