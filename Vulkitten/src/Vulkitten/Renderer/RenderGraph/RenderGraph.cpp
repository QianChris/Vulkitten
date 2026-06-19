#include "vktpch.h"
#include "RenderGraph.h"

namespace Vulkitten {

    void RenderGraph::Execute()
    {
        for (auto& pass : m_Passes)
        {
            std::vector<RenderCommand> passCommands;

            // Filter commands: each pass gets only its relevant command types
            for (auto& cmd : m_FrameCommands)
            {
                bool matches = false;
                if (pass.name == "PreparePass")
                    matches = std::holds_alternative<ClearCommand>(cmd);
                else if (pass.name == "SpriteRenderPass")
                    matches = std::holds_alternative<DrawQuadCommand>(cmd);

                if (matches)
                    passCommands.push_back(cmd);
            }

            if (pass.onExecute)
                pass.onExecute(m_Resources, passCommands, m_BackendContext);
        }

        ClearFrameCommands();
    }
}