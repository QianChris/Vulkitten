#pragma once

#include "Vulkitten/Core/Core.h"

namespace Vulkitten {

    class RenderPass;
    class RenderCommand;

    class RenderGraph {
        std::vector<RenderPass> m_Passes;
        std::vector<RenderCommand> m_FrameCommands;
    };
    
}