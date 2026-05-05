#pragma once

#include <glm/glm.hpp>

#include "Vulkitten/Core/Core.h"
#include "Camera.h"
#include "OrthographicCamera.h"
#include "Texture.h"

namespace Vulkitten {

    struct BatchVertex
    {
        glm::vec3 Position;
        glm::vec4 Color;
        glm::vec2 TexCoord;
        float TexIndex;
        float TilingFactor;
    };

    struct Renderer2DStats
    {
        uint32_t DrawCalls = 0;
        uint32_t Quads = 0;
        uint32_t Vertices = 0;
        uint32_t TextureCount = 0;
    };

    class VKT_API Renderer2D
    {
    public:
        static void Init();
        static void Shutdown();

        static void BeginScene(const OrthographicCamera& camera);
        static void BeginScene(const Camera& camera);
        static void EndScene();

        static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
        static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color);
        static void DrawQuad(const glm::mat4& transform, const glm::vec4& color);

        static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture,
            float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
        static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture,
            float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
        static void DrawQuad(const glm::mat4& transform, const Ref<Texture2D>& texture,
            float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));

        static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color);
        static void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color);
        static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture,
            float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
        static void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture,
            float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));

        static Renderer2DStats& GetStats();
        static uint32_t GetMaxQuads();
        static uint32_t GetMaxTextureSlots();
        static void SetMaxQuads(uint32_t maxQuads);
        static void SetMaxTextureSlots(uint32_t maxSlots);
        static void Flush();
    private:
        static void SortAndFlush();
    };
}