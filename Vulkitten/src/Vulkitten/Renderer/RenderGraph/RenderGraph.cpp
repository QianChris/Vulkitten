#include "vktpch.h"
#include "RenderGraph.h"

#include "Vulkitten/Renderer/Framebuffer.h"

namespace Vulkitten {

    void RenderGraph::SetPassFramebuffer(const std::string& passName, Ref<Framebuffer> fb)
    {
        for (auto& pass : m_Passes)
        {
            if (pass->name == passName)
            {
                pass->SetTargetFramebuffer(fb);
                return;
            }
        }
        VKT_CORE_WARN("RenderGraph::SetPassFramebuffer — pass not found: {0}", passName);
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
                auto fb = pass->GetTargetFramebuffer();
                if (fb)
                    fb->Bind();

                pass->onExecute(m_Resources, passCommands, m_BackendContext);

                if (fb)
                    fb->Unbind();
            }
        }

        ClearFrameCommands();
    }
}
