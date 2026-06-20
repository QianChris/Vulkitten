#pragma once

#include "Vulkitten/Renderer/RenderGraph/RenderPass.h"

#include "Vulkitten/Renderer/Texture.h"
#include "Vulkitten/Renderer/Shader.h"
#include "Vulkitten/Renderer/VertexArray.h"
#include <glm/glm.hpp>
#include <array>

namespace Vulkitten {

class Camera;

// ============================================================
// SpriteRenderPass — 2D quad batch render pass.
//
// Replaces the old static Renderer2D. Owns its own VAO/VBO/IBO,
// shader, and batch state. Executes DrawQuadCommands submitted
// by RenderSystem each frame.
// ============================================================

class SpriteRenderPass : public RenderPass
{
public:
    SpriteRenderPass();
    ~SpriteRenderPass();

private:
    // ---- Internal batch rendering (inlined from Renderer2D) ----

    struct BatchVertex
    {
        glm::vec3 Position;
        glm::vec4 Color;
        glm::vec2 TexCoord;
        float     TexIndex;
        float     TilingFactor;
        int32_t   EntityID;
    };

    struct QuadData
    {
        glm::mat4 Transform{1.0f};
        glm::vec4 Color{1.0f};
        glm::vec2 TexCoords[4];
        float     TexIndex = 0.0f;
        float     TilingFactor = 1.0f;
        float     ZDepth = 0.0f;
        int32_t   EntityID = 0;
    };

    void BeginScene(const Camera& camera);
    void EndScene();
    void SortAndFlush();
    void Flush();

    void DrawQuad(const glm::mat4& transform, const glm::vec4& color, int entityID);
    void DrawQuad(const glm::mat4& transform, const Ref<Texture2D>& texture,
                  float tilingFactor, const glm::vec4& tintColor, int entityID);

    uint32_t GetTextureSlot(const Ref<Texture2D>& texture);

    static constexpr uint32_t s_MaxQuads      = 10000;
    static constexpr uint32_t s_MaxTextureSlots = 32;

    // GPU resources
    Ref<VertexArray>  m_QuadVA;
    Ref<VertexBuffer> m_QuadVB;
    Ref<Shader>       m_TextureShader;
    Ref<Texture2D>    m_WhiteTexture;

    // Batch state
    BatchVertex*      m_VBBase = nullptr;
    BatchVertex*      m_VBPtr  = nullptr;
    uint32_t          m_QuadCount = 0;
    std::vector<QuadData>          m_QuadQueue;
    std::vector<Ref<Texture2D>>    m_TextureSlots;
    uint32_t          m_TexSlotIndex = 1;
};

} // namespace Vulkitten
