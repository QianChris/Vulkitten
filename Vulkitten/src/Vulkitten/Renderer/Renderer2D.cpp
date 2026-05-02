#include "vktpch.h"
#include "Renderer2D.h"

#include "Shader.h"
#include "VertexArray.h"
#include "RenderCommand.h"

#include "Vulkitten/Perf/Instrumentor.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Vulkitten {

    struct Renderer2DData
    {
        ShaderLibrary shaderLibrary;
        Ref<VertexArray> quadVertexArray;
        Ref<Texture2D> whiteTexture;
    };

    static Renderer2DData* s_Data;

    void Renderer2D::Init()
    {
        VKT_PROFILE_FUNCTION();

        s_Data = new Renderer2DData();

        s_Data->shaderLibrary.Load("sandbox://assets/shaders/Texture.shader");
        auto textureShader = s_Data->shaderLibrary.Get("Texture");
        textureShader->Bind();
        textureShader->SetUniformInt("u_Texture", 0);

        s_Data->quadVertexArray = VertexArray::Create();
        {
            float vertices[4 * 5] = {
                -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
                0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
                0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
                -0.5f,  0.5f, 0.0f, 0.0f, 1.0f,
            };
            Ref<VertexBuffer> vertexBuffer;
            vertexBuffer = VertexBuffer::Create(vertices, sizeof(vertices));

            unsigned int indices[6] = { 0, 1, 2, 2, 3, 0 };
            Ref<IndexBuffer> indexBuffer;
            indexBuffer = IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));

            BufferLayout layout = {
                { ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" },
            };
            vertexBuffer->SetLayout(layout);
            s_Data->quadVertexArray->AddVertexBuffer(vertexBuffer);
            s_Data->quadVertexArray->SetIndexBuffer(indexBuffer);
        }

        s_Data->whiteTexture = Texture2D::Create(1, 1);
        uint32_t whiteTextureData = 0xffffffff;
        s_Data->whiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));
    }

    void Renderer2D::Shutdown()
    {
        VKT_PROFILE_FUNCTION();

        delete s_Data;
    }

    void Renderer2D::BeginScene(const OrthographicCamera& camera)
    {
        VKT_PROFILE_FUNCTION();

        auto textureShader = s_Data->shaderLibrary.Get("Texture");
        textureShader->Bind();
        textureShader->SetUniformMat4("u_ViewProjection", camera.GetViewProjectionMatrix());
    }

    void Renderer2D::EndScene()
    {
        VKT_PROFILE_FUNCTION();

    }

    void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
    {
        VKT_PROFILE_FUNCTION();

        DrawQuad(glm::vec3(position, 0.0f), size, color);
    }

    void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
    {
        VKT_PROFILE_FUNCTION();

        s_Data->whiteTexture->Bind();

        auto texturedShader = s_Data->shaderLibrary.Get("Texture");
        texturedShader->Bind();
        texturedShader->SetUniformFloat4("u_TintColor", color);

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
            * glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));
        texturedShader->SetUniformMat4("u_Transform", transform);

        s_Data->quadVertexArray->Bind();
        RenderCommand::DrawIndexed(s_Data->quadVertexArray);
    }

    void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
    {
        VKT_PROFILE_FUNCTION();

        texture->Bind();

        auto texturedShader = s_Data->shaderLibrary.Get("Texture");
        texturedShader->Bind();
        texturedShader->SetUniformFloat4("u_TintColor", tintColor);

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f))
            * glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));
        texturedShader->SetUniformMat4("u_Transform", transform);

        texturedShader->SetUniformFloat("u_TilingFactor", tilingFactor);

        s_Data->quadVertexArray->Bind();
        RenderCommand::DrawIndexed(s_Data->quadVertexArray);
    }

}