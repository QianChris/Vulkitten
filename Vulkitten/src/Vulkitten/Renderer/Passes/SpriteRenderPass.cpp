#include "vktpch.h"
#include "SpriteRenderPass.h"

#include "Vulkitten/Renderer/IRenderer.h"
#include "Vulkitten/Renderer/Backend/OpenGL/OpenGLRenderer.h"
#include "Vulkitten/Renderer/RenderGraph/RenderGraph.h"
#include "Vulkitten/Renderer/Camera.h"
#include "Vulkitten/Renderer/Framebuffer.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Vulkitten {

// ============================================================
// Construction / Destruction
// ============================================================

SpriteRenderPass::SpriteRenderPass()
{
    name = "SpriteRenderPass";

    // ---- Create GPU resources ----
    m_TextureShader = Shader::Create("engine://shaders/TextureEntity");
    m_TextureShader->Bind();
    for (uint32_t i = 0; i < s_MaxTextureSlots; i++)
        m_TextureShader->SetUniformInt(("u_Textures[" + std::to_string(i) + "]").c_str(), i);

    m_QuadVA = VertexArray::Create();

    uint32_t maxVertices = s_MaxQuads * 4;
    m_QuadVB = VertexBuffer::Create(maxVertices * sizeof(BatchVertex));
    m_QuadVB->SetLayout({
        { ShaderDataType::Float3, "a_Position" },
        { ShaderDataType::Float4, "a_Color" },
        { ShaderDataType::Float2, "a_TexCoord" },
        { ShaderDataType::Float,  "a_TexIndex" },
        { ShaderDataType::Float,  "a_TilingFactor" },
        { ShaderDataType::Int,    "a_EntityID" }
    });
    m_QuadVA->AddVertexBuffer(m_QuadVB);

    uint32_t maxIndices = s_MaxQuads * 6;
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
    Ref<IndexBuffer> ib = IndexBuffer::Create(indices.data(), maxIndices);
    m_QuadVA->SetIndexBuffer(ib);

    m_VBBase = new BatchVertex[maxVertices];

    m_TextureSlots.resize(s_MaxTextureSlots);
    m_WhiteTexture = Texture2D::Create(1, 1);
    uint32_t white = 0xffffffff;
    m_WhiteTexture->SetData(&white, sizeof(uint32_t));
    m_TextureSlots[0] = m_WhiteTexture;

    m_QuadQueue.reserve(s_MaxQuads);

    // ---- Build the execute callback ----
    SetExecute([this](const std::vector<RenderGraphResource>& /*resources*/,
                      const std::vector<RenderCommand>& commands,
                      void* /*backendContext*/) {
        if (commands.empty())
            return;

        const auto& perFrame = IRenderer::Get().GetRenderGraph()->GetPerFrameData();
        auto* camera = perFrame.Camera;
        if (!camera)
            return;

        // Bind the configured Framebuffer (nullptr = default backbuffer)
        auto* graph = GetGraph();
        auto fb = graph ? graph->GetFramebuffer("Viewport") : nullptr;
        if (fb)
            fb->Bind();

        BeginScene(*camera);

        for (auto& cmd : commands)
        {
            if (auto* drawCmd = std::get_if<DrawQuadCommand>(&cmd))
            {
                if (drawCmd->texture)
                {
                    DrawQuad(drawCmd->transform, drawCmd->texture,
                             drawCmd->tilingFactor, drawCmd->color, 0);
                }
                else
                {
                    DrawQuad(drawCmd->transform, drawCmd->color, 0);
                }
            }
        }

        EndScene();

        if (fb)
            fb->Unbind();
    });
}

SpriteRenderPass::~SpriteRenderPass()
{
    delete[] m_VBBase;
}

// ============================================================
// Scene Management
// ============================================================

void SpriteRenderPass::BeginScene(const Camera& camera)
{
    m_TextureShader->Bind();
    m_TextureShader->SetUniformMat4("u_ViewProjection", camera.GetViewProjectionMatrix());

    m_QuadCount = 0;
    m_VBPtr = m_VBBase;
    m_TexSlotIndex = 1;
    m_QuadQueue.clear();
}

void SpriteRenderPass::EndScene()
{
    SortAndFlush();
}

// ============================================================
// DrawQuad Variants
// ============================================================

void SpriteRenderPass::DrawQuad(const glm::mat4& transform, const glm::vec4& color, int entityID)
{
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
    quad.EntityID = entityID;
    m_QuadQueue.push_back(quad);
}

void SpriteRenderPass::DrawQuad(const glm::mat4& transform, const Ref<Texture2D>& texture,
                                 float tilingFactor, const glm::vec4& tintColor, int entityID)
{
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
    quad.EntityID = entityID;
    m_QuadQueue.push_back(quad);
}

// ============================================================
// Batch Render
// ============================================================

void SpriteRenderPass::SortAndFlush()
{
    if (m_QuadQueue.empty())
        return;

    for (const auto& quad : m_QuadQueue)
    {
        if (m_QuadCount >= s_MaxQuads)
        {
            Flush();
            m_QuadCount = 0;
        }

        glm::vec3 localPositions[4] = {
            glm::vec3(-0.5f, -0.5f, 0.0f),
            glm::vec3( 0.5f, -0.5f, 0.0f),
            glm::vec3( 0.5f,  0.5f, 0.0f),
            glm::vec3(-0.5f,  0.5f, 0.0f)
        };

        for (int i = 0; i < 4; i++)
        {
            m_VBPtr->Position = quad.Transform * glm::vec4(localPositions[i], 1.0f);
            m_VBPtr->Color = quad.Color;
            m_VBPtr->TexCoord = quad.TexCoords[i];
            m_VBPtr->TexIndex = quad.TexIndex;
            m_VBPtr->TilingFactor = quad.TilingFactor;
            m_VBPtr->EntityID = quad.EntityID;
            m_VBPtr++;
        }

        m_QuadCount++;
    }

    Flush();
}

void SpriteRenderPass::Flush()
{
    if (m_QuadCount == 0)
        return;

    uint32_t dataSize = (uint32_t)((uint8_t*)m_VBPtr - (uint8_t*)m_VBBase);
    m_QuadVB->SetData(m_VBBase, dataSize);

    for (uint32_t i = 0; i < m_TexSlotIndex; i++)
        m_TextureSlots[i]->Bind(i);

    m_QuadVA->Bind();

    // Use RendererAPI directly instead of Legacy::RenderCommand
    auto* api = static_cast<OpenGLRenderer&>(IRenderer::Get()).GetRendererAPI();
    api->DrawIndexed(m_QuadVA, m_QuadCount * 6);

    m_QuadCount = 0;
    m_VBPtr = m_VBBase;
    m_TexSlotIndex = 1;
}

// ============================================================
// Texture Slot Management
// ============================================================

uint32_t SpriteRenderPass::GetTextureSlot(const Ref<Texture2D>& texture)
{
    for (uint32_t i = 1; i < m_TexSlotIndex; i++)
    {
        if (m_TextureSlots[i].get() == texture.get())
            return i;
    }

    if (m_TexSlotIndex >= s_MaxTextureSlots)
        m_TexSlotIndex = 1;

    uint32_t slot = m_TexSlotIndex;
    m_TextureSlots[slot] = texture;
    m_TexSlotIndex++;
    return slot;
}

} // namespace Vulkitten
