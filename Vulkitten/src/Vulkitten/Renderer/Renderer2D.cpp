#include "vktpch.h"
#include "Renderer2D.h"

#include "Shader.h"
#include "VertexArray.h"
#include "Buffer.h"
#include "RenderCommand.h"

#include "Vulkitten/Perf/Instrumentor.h"

#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <algorithm>

namespace Vulkitten {

    static uint32_t s_MaxQuads = 10000;
    static uint32_t s_MaxTextureSlots = 32;

    struct QuadData
    {
        glm::mat4 Transform;
        glm::vec4 Color;
        glm::vec2 TexCoords[4];
        float TexIndex;
        float TilingFactor;
        float ZDepth;
    };

    struct Renderer2DData
    {
        Ref<VertexArray> quadVertexArray;
        Ref<VertexBuffer> quadVertexBuffer;
        Ref<Shader> textureShader;
        Ref<Texture2D> whiteTexture;

        uint32_t quadCount = 0;
        BatchVertex* vertexBufferBase = nullptr;
        BatchVertex* vertexBufferPtr = nullptr;

        std::vector<QuadData> quadQueue;
        std::vector<Ref<Texture2D>> textureSlots;
        uint32_t textureSlotIndex = 1;

        Renderer2DStats stats;
    };

    static Renderer2DData* s_Data;

    static glm::vec4 GetColorQuadPositions(const glm::vec2& position, const glm::vec2& size)
    {
        return glm::vec4(position.x, position.y, position.x + size.x, position.y + size.y);
    }

    void Renderer2D::Init()
    {
        VKT_PROFILE_FUNCTION();

        s_Data = new Renderer2DData();

        s_Data->quadQueue.reserve(s_MaxQuads);
        s_Data->textureSlots.resize(s_MaxTextureSlots);

        s_Data->textureShader = Shader::Create("sandbox://assets/shaders/Texture.shader");
        s_Data->textureShader->Bind();
        for (uint32_t i = 0; i < s_MaxTextureSlots; i++)
        {
            s_Data->textureShader->SetUniformInt(("u_Textures[" + std::to_string(i) + "]").c_str(), i);
        }

        s_Data->quadVertexArray = VertexArray::Create();

        uint32_t maxVertices = s_MaxQuads * 4;
        uint32_t maxIndices = s_MaxQuads * 6;
        s_Data->quadVertexBuffer = VertexBuffer::Create(maxVertices * sizeof(BatchVertex));
        s_Data->quadVertexBuffer->SetLayout({
            { ShaderDataType::Float3, "a_Position" },
            { ShaderDataType::Float4, "a_Color" },
            { ShaderDataType::Float2, "a_TexCoord" },
            { ShaderDataType::Float, "a_TexIndex" },
            { ShaderDataType::Float, "a_TilingFactor" }
        });
        s_Data->quadVertexArray->AddVertexBuffer(s_Data->quadVertexBuffer);

        std::vector<uint32_t> indices(maxIndices);
        uint32_t offset = 0;
        for (uint32_t i = 0; i < maxIndices; i += 6)
        {
            indices[i + 0] = offset + 0;
            indices[i + 1] = offset + 1;
            indices[i + 2] = offset + 2;
            indices[i + 3] = offset + 2;
            indices[i + 4] = offset + 3;
            indices[i + 5] = offset + 0;
            offset += 4;
        }
        Ref<IndexBuffer> indexBuffer = IndexBuffer::Create(indices.data(), maxIndices);
        s_Data->quadVertexArray->SetIndexBuffer(indexBuffer);

        s_Data->vertexBufferBase = new BatchVertex[maxVertices];

        s_Data->whiteTexture = Texture2D::Create(1, 1);
        uint32_t whiteTextureData = 0xffffffff;
        s_Data->whiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));
        s_Data->textureSlots[0] = s_Data->whiteTexture;
    }

    void Renderer2D::Shutdown()
    {
        VKT_PROFILE_FUNCTION();

        delete[] s_Data->vertexBufferBase;
        delete s_Data;
    }

    void Renderer2D::BeginScene(const OrthographicCamera& camera)
    {
        VKT_PROFILE_FUNCTION();

        s_Data->textureShader->Bind();
        s_Data->textureShader->SetUniformMat4("u_ViewProjection", camera.GetViewProjectionMatrix());

        s_Data->quadCount = 0;
        s_Data->vertexBufferPtr = s_Data->vertexBufferBase;
        s_Data->textureSlotIndex = 1;
        s_Data->quadQueue.clear();

        s_Data->stats.DrawCalls = 0;
        s_Data->stats.Quads = 0;
        s_Data->stats.Vertices = 0;
        s_Data->stats.TextureCount = 1;
    }

void Renderer2D::BeginScene(const Camera& camera)
    {
        VKT_PROFILE_FUNCTION();

        s_Data->textureShader->Bind();
        s_Data->textureShader->SetUniformMat4("u_ViewProjection", camera.GetViewProjectionMatrix());

        s_Data->quadCount = 0;
        s_Data->vertexBufferPtr = s_Data->vertexBufferBase;
        s_Data->textureSlotIndex = 1;
        s_Data->quadQueue.clear();

        s_Data->stats.DrawCalls = 0;
        s_Data->stats.Quads = 0;
        s_Data->stats.Vertices = 0;
        s_Data->stats.TextureCount = 1;
    }

    void Renderer2D::EndScene()
    {
        VKT_PROFILE_FUNCTION();

        SortAndFlush();
    }

    void Renderer2D::SortAndFlush()
    {
        if (s_Data->quadQueue.empty())
            return;

        std::sort(s_Data->quadQueue.begin(), s_Data->quadQueue.end(),
            [](const QuadData& a, const QuadData& b) {
                return a.ZDepth < b.ZDepth;
            });

        uint32_t batchQuadCount = 0;

        for (const auto& quad : s_Data->quadQueue)
        {
            if (batchQuadCount >= s_MaxQuads)
            {
                Flush();
                batchQuadCount = 0;
            }

            glm::vec3 localPositions[4] = {
                glm::vec3(-0.5f, -0.5f, 0.0f),
                glm::vec3( 0.5f, -0.5f, 0.0f),
                glm::vec3( 0.5f,  0.5f, 0.0f),
                glm::vec3(-0.5f,  0.5f, 0.0f)
            };

            for (int i = 0; i < 4; i++)
            {
                s_Data->vertexBufferPtr->Position = quad.Transform * glm::vec4(localPositions[i], 1.0f);
                s_Data->vertexBufferPtr->Color = quad.Color;
                s_Data->vertexBufferPtr->TexCoord = quad.TexCoords[i];
                s_Data->vertexBufferPtr->TexIndex = quad.TexIndex;
                s_Data->vertexBufferPtr->TilingFactor = quad.TilingFactor;
                s_Data->vertexBufferPtr++;
            }

            s_Data->quadCount++;
            batchQuadCount++;
        }

        Flush();
    }

    void Renderer2D::Flush()
    {
        if (s_Data->quadCount == 0)
            return;

        uint32_t dataSize = (uint32_t)((uint8_t*)s_Data->vertexBufferPtr - (uint8_t*)s_Data->vertexBufferBase);
        s_Data->quadVertexBuffer->SetData(s_Data->vertexBufferBase, dataSize);

        for (uint32_t i = 0; i < s_Data->textureSlotIndex; i++)
        {
            s_Data->textureSlots[i]->Bind(i);
        }

        s_Data->quadVertexArray->Bind();
        RenderCommand::DrawIndexed(s_Data->quadVertexArray, s_Data->quadCount * 6);

        s_Data->stats.DrawCalls++;
        s_Data->stats.Quads += s_Data->quadCount;
        s_Data->stats.Vertices += s_Data->quadCount * 4;
        s_Data->stats.TextureCount = std::max(s_Data->stats.TextureCount, s_Data->textureSlotIndex);

        s_Data->quadCount = 0;
        s_Data->vertexBufferPtr = s_Data->vertexBufferBase;
        s_Data->textureSlotIndex = 1;
    }

    Renderer2DStats& Renderer2D::GetStats()
    {
        return s_Data->stats;
    }

    uint32_t Renderer2D::GetMaxQuads()
    {
        return s_MaxQuads;
    }

    uint32_t Renderer2D::GetMaxTextureSlots()
    {
        return s_MaxTextureSlots;
    }

    void Renderer2D::SetMaxQuads(uint32_t maxQuads)
    {
        s_MaxQuads = maxQuads;
    }

    void Renderer2D::SetMaxTextureSlots(uint32_t maxSlots)
    {
        s_MaxTextureSlots = maxSlots;
    }

    static uint32_t GetTextureSlot(const Ref<Texture2D>& texture)
    {
        for (uint32_t i = 1; i < s_Data->textureSlotIndex; i++)
        {
            if (s_Data->textureSlots[i].get() == texture.get())
                return i;
        }

        if (s_Data->textureSlotIndex >= s_MaxTextureSlots)
        {
            s_Data->textureSlotIndex = 1;
        }

        uint32_t slot = s_Data->textureSlotIndex;
        s_Data->textureSlots[slot] = texture;
        s_Data->textureSlotIndex++;
        s_Data->stats.TextureCount++;
        return slot;
    }

    static void CheckQuadCountOverflow()
    {
    }

    void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
    {
        DrawQuad(glm::vec3(position, 0.0f), size, color);
    }

    void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) *
            glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));
        DrawQuad(transform, color);
    }

    void Renderer2D::DrawQuad(const glm::mat4& transform, const glm::vec4& color)
    {
        VKT_PROFILE_FUNCTION();

        QuadData quad;
        quad.Transform = transform;
        quad.Color = color;
        quad.TexCoords[0] = { 0.0f, 0.0f };
        quad.TexCoords[1] = { 1.0f, 0.0f };
        quad.TexCoords[2] = { 1.0f, 1.0f };
        quad.TexCoords[3] = { 0.0f, 1.0f };
        quad.TexIndex = 0.0f;
        quad.TilingFactor = 1.0f;
        quad.ZDepth = transform[3].z;

        s_Data->quadQueue.push_back(quad);
    }

    void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
    {
        DrawQuad(glm::vec3(position, 0.0f), size, texture, tilingFactor, tintColor);
    }

    void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
    {
        VKT_PROFILE_FUNCTION();

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) *
            glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));

        QuadData quad;
        quad.Transform = transform;
        quad.Color = tintColor;
        quad.TexCoords[0] = { 0.0f, 0.0f };
        quad.TexCoords[1] = { 1.0f, 0.0f };
        quad.TexCoords[2] = { 1.0f, 1.0f };
        quad.TexCoords[3] = { 0.0f, 1.0f };
        quad.TexIndex = (float)GetTextureSlot(texture);
        quad.TilingFactor = tilingFactor;
        quad.ZDepth = transform[3].z;

        s_Data->quadQueue.push_back(quad);
    }

    void Renderer2D::DrawQuad(const glm::mat4& transform, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
    {
        VKT_PROFILE_FUNCTION();

        QuadData quad;
        quad.Transform = transform;
        quad.Color = tintColor;
        quad.TexCoords[0] = { 0.0f, 0.0f };
        quad.TexCoords[1] = { 1.0f, 0.0f };
        quad.TexCoords[2] = { 1.0f, 1.0f };
        quad.TexCoords[3] = { 0.0f, 1.0f };
        quad.TexIndex = (float)GetTextureSlot(texture);
        quad.TilingFactor = tilingFactor;
        quad.ZDepth = transform[3].z;

        s_Data->quadQueue.push_back(quad);
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
    {
        DrawRotatedQuad(glm::vec3(position, 0.0f), size, rotation, color);
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
    {
        VKT_PROFILE_FUNCTION();

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
            * glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f))
            * glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));

        QuadData quad;
        quad.Transform = transform;
        quad.Color = color;
        quad.TexCoords[0] = { 0.0f, 0.0f };
        quad.TexCoords[1] = { 1.0f, 0.0f };
        quad.TexCoords[2] = { 1.0f, 1.0f };
        quad.TexCoords[3] = { 0.0f, 1.0f };
        quad.TexIndex = 0.0f;
        quad.TilingFactor = 1.0f;
        quad.ZDepth = transform[3].z;

        s_Data->quadQueue.push_back(quad);
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
    {
        DrawRotatedQuad(glm::vec3(position, 0.0f), size, rotation, texture, tilingFactor, tintColor);
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
    {
        VKT_PROFILE_FUNCTION();

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
            * glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f))
            * glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));

        QuadData quad;
        quad.Transform = transform;
        quad.Color = tintColor;
        quad.TexCoords[0] = { 0.0f, 0.0f };
        quad.TexCoords[1] = { 1.0f, 0.0f };
        quad.TexCoords[2] = { 1.0f, 1.0f };
        quad.TexCoords[3] = { 0.0f, 1.0f };
        quad.TexIndex = (float)GetTextureSlot(texture);
        quad.TilingFactor = tilingFactor;
        quad.ZDepth = transform[3].z;

        s_Data->quadQueue.push_back(quad);
    }

}