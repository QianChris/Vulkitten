#include "vktpch.h"
#include "OpenGLRendererAPI.h"

#include "Vulkitten/Perf/Instrumentor.h"

#include <glad/glad.h>

namespace Vulkitten {
    void OpenGLRendererAPI::Init()
    {
        VKT_PROFILE_RENDER_FUNCTION();

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    void OpenGLRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
        glViewport(x, y, width, height);
    }
    void OpenGLRendererAPI::SetClearColor(const glm::vec4& color)
    {
        VKT_PROFILE_RENDER_FUNCTION();

        glClearColor(color.r, color.g, color.b, color.a);
    }

    void OpenGLRendererAPI::Clear()
    {
        VKT_PROFILE_RENDER_FUNCTION();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void OpenGLRendererAPI::DrawIndexed(const Ref<VertexArray>& vertexArray)
    {
        VKT_PROFILE_RENDER_FUNCTION();

        glDrawElements(GL_TRIANGLES, vertexArray->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, nullptr);
    }
}