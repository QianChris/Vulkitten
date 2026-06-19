#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Renderer/RendererAPI.h"
#include "Vulkitten/Renderer/OrthographicCamera.h"
//#include "Vulkitten/Renderer/Shader.h"

#include "Vulkitten/Renderer/RenderGraph/RenderGraph.h"

namespace Vulkitten {

    class Shader;
    class VertexArray;

    class VKT_API Renderer
    { 
    public:
        static void Init();
        static void OnWindowResize(uint32_t width, uint32_t height);
        static void Shutdown();
        static void BeginScene(OrthographicCamera& camera);
        static void EndScene();

        static void Submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray, const glm::mat4& transform = glm::mat4(1.0f));

        inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

        // RenderGraph
        inline static RenderGraph* GetRenderGraph() { return m_graph; }
        static void Render();
        inline static void SetViewProjection(const glm::mat4& vp) { if (m_graph) m_graph->SetViewProjection(vp); }

    private:
        struct SceneData
        {
            glm::mat4 ViewProjectionMatrix;
        };
        static SceneData* s_SceneData;

        static RenderGraph* m_graph;
    };
}