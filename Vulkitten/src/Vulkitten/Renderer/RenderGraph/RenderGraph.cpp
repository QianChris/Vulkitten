#include "vktpch.h"
#include "RenderGraph.h"

#include "Vulkitten/Renderer/Framebuffer.h"

namespace Vulkitten {

    void RenderGraph::SetFramebuffer(const std::string& key, Ref<Framebuffer> fb)
    {
        m_FramebufferMap[key] = fb;
    }

    Ref<Framebuffer> RenderGraph::GetFramebuffer(const std::string& key) const
    {
        auto it = m_FramebufferMap.find(key);
        if (it != m_FramebufferMap.end())
            return it->second;
        return nullptr;
    }

    void RenderGraph::ResizeAllFramebuffers(uint32_t width, uint32_t height)
    {
        for (auto& [key, fb] : m_FramebufferMap)
        {
            if (fb)
            {
                fb->Resize(width, height);
            }
        }
    }

    void RenderGraph::Execute()
    {
        for (auto& pass : m_Passes)
        {
            std::vector<RenderCommand> passCommands;

            for (auto& cmd : m_FrameCommands)
            {
                bool matches = false;
                if (pass->name == "PreparePass")
                    matches = std::holds_alternative<ClearCommand>(cmd);
                else if (pass->name == "SpriteRenderPass")
                    matches = std::holds_alternative<DrawQuadCommand>(cmd);

                if (matches)
                    passCommands.push_back(cmd);
            }

            if (pass->onExecute)
            {
                pass->onExecute(m_Resources, passCommands, m_BackendContext);
            }
        }

        ClearFrameCommands();
    }
}
