#pragma once

#include "Vulkitten/Core.h"
#include "Vulkitten/Renderer/RendererAPI.h"
#include "Vulkitten/Renderer/OrthographicCamera.h"

namespace Vulkitten {

    class Shader;
    class VertexArray;

    class VKT_API Renderer
    { 
    public:
        static void BeginScene(OrthographicCamera& camera);
        static void EndScene();

        static void Submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray, const glm::mat4& transform = glm::mat4(1.0f));

        inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }
    private:
        struct SceneData
        {
            glm::mat4 ViewProjectionMatrix;
        };
        static SceneData* s_SceneData;
    };
}