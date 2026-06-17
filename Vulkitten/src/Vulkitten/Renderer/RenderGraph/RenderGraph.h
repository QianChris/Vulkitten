#pragma once

#include "Vulkitten/Core/Core.h"

#include "Vulkitten/Renderer/RenderGraph/RenderCommand.h"
#include "Vulkitten/Renderer/RenderGraph/RenderPass.h"

namespace Vulkitten {

    class VKT_API RenderGraph {
    public:
        std::vector<RenderPass> m_Passes{};
        std::vector<RenderCommand> m_FrameCommands{};
    };
    
}