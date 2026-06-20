#pragma once

#include "Vulkitten/Core/Core.h"

#include "Vulkitten/Renderer/RenderGraph/RenderCommand.h"
#include "Vulkitten/Renderer/RenderGraph/RenderPass.h"
#include "Vulkitten/Renderer/Camera.h"

#include <glm/glm.hpp>

namespace Vulkitten {

    class Scene;

    // ============================================================
    // PerFrameSceneData — per-frame data populated by RenderSystem
    // and consumed by Passes during Execute().
    // ============================================================
    struct VKT_API PerFrameSceneData
    {
        class Camera* Camera = nullptr;
        glm::mat4 ViewProjection{1.0f};
        class Scene* Scene = nullptr;

        // Future: LightData, environment map, scene bounds, etc.
        struct LightData {}; // Placeholder for light list
        LightData Lights;
    };

    class VKT_API RenderGraph {
    public:
        RenderGraph() = default;
        ~RenderGraph() = default;

        void AddPass(const RenderPass& pass) {
            m_Passes.push_back(pass);
        }
        void AddCommand(const RenderCommand& command) {
            m_FrameCommands.push_back(command);
        }

        void Execute();

        void SetBackendContext(void* context) { m_BackendContext = context; }

        // ---- Per-Frame Scene Data ----
        void SetPerFrameData(const PerFrameSceneData& data) { m_PerFrameData = data; }
        const PerFrameSceneData& GetPerFrameData() const { return m_PerFrameData; }

        // ---- Pass Framebuffer configuration ----
        void SetPassFramebuffer(const std::string& passName, Ref<class Framebuffer> fb);

        // ---- Query registered pass count and names (for debug UI) ----
        uint32_t GetPassCount() const { return static_cast<uint32_t>(m_Passes.size()); }
        const std::string& GetPassName(uint32_t index) const {
            VKT_CORE_ASSERT(index < m_Passes.size(), "GetPassName: index out of range");
            return m_Passes[index].name;
        }

    private:
        void ClearFrameCommands() {
            m_FrameCommands.clear();
        }

        std::vector<RenderPass> m_Passes{};
        std::vector<RenderCommand> m_FrameCommands{};
        std::vector<RenderGraphResource> m_Resources{};
        void* m_BackendContext = nullptr;

        PerFrameSceneData m_PerFrameData;
    };

}
