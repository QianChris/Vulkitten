#pragma once

#include "Vulkitten/Core/Core.h"

#include "Vulkitten/Renderer/RenderGraph/RenderCommand.h"
#include "Vulkitten/Renderer/RenderGraph/RenderPass.h"

#include <glm/glm.hpp>

namespace Vulkitten {

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
        void SetViewProjection(const glm::mat4& vp) { m_ViewProjection = vp; }
        const glm::mat4& GetViewProjection() const { return m_ViewProjection; }

        // Camera for passes that need it (e.g., SpriteRenderPass)
        void SetSceneCamera(class Camera* camera) { m_SceneCamera = camera; }
        class Camera* GetSceneCamera() const { return m_SceneCamera; }

        // Framebuffer for passes that render offscreen (nullptr = default backbuffer)
        void SetFramebuffer(Ref<class Framebuffer> fb) { m_Framebuffer = fb; }
        Ref<class Framebuffer> GetFramebuffer() const { return m_Framebuffer; }

        // Scene for passes that need ECS access (e.g., GpuParticlePass)
        void SetScene(class Scene* scene) { m_Scene = scene; }
        class Scene* GetScene() const { return m_Scene; }

        // Query registered pass count and names (for debug UI, editor panels, etc.)
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
        glm::mat4 m_ViewProjection{1.0f};
        class Camera* m_SceneCamera = nullptr;
        Ref<class Framebuffer> m_Framebuffer;
        class Scene* m_Scene = nullptr;
    };

}