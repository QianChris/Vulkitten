#pragma once

#include "Vulkitten/Core/Core.h"

#include "Vulkitten/Renderer/RenderGraph/RenderCommand.h"
#include "Vulkitten/Renderer/RenderGraph/RenderPass.h"
#include "Vulkitten/Renderer/Camera.h"

#include <glm/glm.hpp>
#include <unordered_map>

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

        struct LightData {};
        LightData Lights;
    };

    class VKT_API RenderGraph {
    public:
        RenderGraph() = default;
        ~RenderGraph() = default;

        // Takes ownership of the pass (avoids slicing of derived Pass types).
        void AddPass(Ref<RenderPass> pass) {
            pass->m_Graph = this;
            m_Passes.push_back(std::move(pass));
        }
        void AddCommand(const RenderCommand& command) {
            m_FrameCommands.push_back(command);
        }

        void Execute();

        void SetBackendContext(void* context) { m_BackendContext = context; }
        void* GetBackendContext() const { return m_BackendContext; }

        // ---- Per-Frame Scene Data ----
        void SetPerFrameData(const PerFrameSceneData& data) { m_PerFrameData = data; }
        const PerFrameSceneData& GetPerFrameData() const { return m_PerFrameData; }

        // ---- Key-Value Framebuffer Map ----
        // Passes retrieve FBs by key (e.g. "Viewport") via GetFramebuffer().
        // On window resize, all registered FBs are resized automatically.
        void SetFramebuffer(const std::string& key, Ref<class Framebuffer> fb);
        Ref<class Framebuffer> GetFramebuffer(const std::string& key) const;
        void ResizeAllFramebuffers(uint32_t width, uint32_t height);

        // ---- Query registered pass count and names (for debug UI) ----
        uint32_t GetPassCount() const { return static_cast<uint32_t>(m_Passes.size()); }
        const std::string& GetPassName(uint32_t index) const {
            VKT_CORE_ASSERT(index < m_Passes.size(), "GetPassName: index out of range");
            return m_Passes[index]->name;
        }

    private:
        void ClearFrameCommands() {
            m_FrameCommands.clear();
        }

        std::vector<Ref<RenderPass>> m_Passes{};
        std::vector<RenderCommand>     m_FrameCommands{};
        std::vector<RenderGraphResource> m_Resources{};
        std::unordered_map<std::string, Ref<class Framebuffer>> m_FramebufferMap;
        void* m_BackendContext = nullptr;

        PerFrameSceneData m_PerFrameData;
    };

}
