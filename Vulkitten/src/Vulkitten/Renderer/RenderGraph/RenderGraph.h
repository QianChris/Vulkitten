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
    };

}