#pragma once

#include "Vulkitten/Core/Core.h"

#include "Vulkitten/Renderer/RenderGraph/RenderCommand.h"
#include "Vulkitten/Renderer/RenderGraph/RenderPass.h"

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

    private:
        void Clear() {
            m_Passes.clear();
			m_FrameCommands.clear();
        }

        std::vector<RenderPass> m_Passes{};
        std::vector<RenderCommand> m_FrameCommands{};
    };
    
}