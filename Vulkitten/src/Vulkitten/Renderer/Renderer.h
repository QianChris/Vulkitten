#pragma once

#include "Vulkitten/Core.h"
#include "Vulkitten/Renderer/RendererAPI.h"

namespace Vulkitten {

    class VKT_API Renderer
    { 
    public:
        static void BeginScene();
        static void EndScene();

        static void Submit(const std::shared_ptr<VertexArray>& vertexArray);

        inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }
    };
}