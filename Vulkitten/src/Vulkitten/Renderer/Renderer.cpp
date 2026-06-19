#include "vktpch.h"
#include "Renderer.h"
#include "RenderCommand.h"
#include "Shader.h"
#include "Renderer2D.h"

#include "Vulkitten/Renderer/Passes/PreparePass.h"
#include "Vulkitten/Renderer/Passes/GpuParticlePass.h"
#include "Vulkitten/Renderer/Passes/SpriteRenderPass.h"
#include "Vulkitten/Renderer/Passes/EndPass.h"
#include "Vulkitten/Core/Application.h"

#include "Vulkitten/Perf/Instrumentor.h"

namespace Vulkitten {

    Renderer::SceneData* Renderer::s_SceneData = new Renderer::SceneData();
	RenderGraph* Renderer::m_graph = nullptr;

    void Renderer::Init()
    {
        VKT_PROFILE_FUNCTION();

        Legacy::RenderCommand::Init();
        Renderer2D::Init();

        m_graph = new RenderGraph();

        // Register default render passes (order matters)
        m_graph->AddPass(PreparePass{});         // Clear
        m_graph->AddPass(GpuParticlePass{});     // GPU particle update + render
        m_graph->AddPass(SpriteRenderPass{});    // 2D quad batch
        m_graph->AddPass(EndPass{});             // SwapBuffers

        // Set backend context for EndPass (SwapBuffers)
        m_graph->SetBackendContext(Application::Get().GetWindow().GetGraphicsContext());
    }

    void Renderer::OnWindowResize(uint32_t width, uint32_t height)
    {
        VKT_PROFILE_FUNCTION();

        Legacy::RenderCommand::SetViewport(0, 0, width, height);
    }

    void Renderer::Shutdown()
    {
        VKT_PROFILE_FUNCTION();

        delete m_graph;

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
        Legacy::RenderCommand::DrawIndexed(vertexArray);
    }

    void Renderer::Render()
    {
        m_graph->Execute();
    }
}