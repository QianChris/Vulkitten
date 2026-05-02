#include "vktpch.h"
#include "Renderer.h"
#include "RenderCommand.h"
#include "Shader.h"
#include "Renderer2D.h"

#include "Vulkitten/Perf/Instrumentor.h"

namespace Vulkitten {

    Renderer::SceneData* Renderer::s_SceneData = new Renderer::SceneData();

    void Renderer::Init()
    {
        VKT_PROFILE_FUNCTION();

        RenderCommand::Init();
        Renderer2D::Init();
    }

    void Renderer::OnWindowResize(uint32_t width, uint32_t height)
    {
        VKT_PROFILE_FUNCTION();

        RenderCommand::SetViewport(0, 0, width, height);
    }

    void Renderer::Shutdown()
    {
        VKT_PROFILE_FUNCTION();

        Renderer2D::Shutdown();
    }

    void Renderer::BeginScene(OrthographicCamera& camera)
    {
        VKT_PROFILE_FUNCTION();

        s_SceneData->ViewProjectionMatrix = camera.GetViewProjectionMatrix();
    }

    void Renderer::EndScene()
    {
        VKT_PROFILE_FUNCTION();

    }

    void Renderer::Submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray, const glm::mat4& transform)
    {
        VKT_PROFILE_FUNCTION();

        shader->Bind();
        shader->SetUniformMat4("u_ViewProjection", s_SceneData->ViewProjectionMatrix);
        shader->SetUniformMat4("u_Transform", transform);

        vertexArray->Bind();
        RenderCommand::DrawIndexed(vertexArray);
    }

}