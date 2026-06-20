#include "vktpch.h"
#include "VkRenderContext.h"

namespace Vulkitten {

VkRenderContext::VkRenderContext(FrameContext& frameContext, IRenderer& renderer)
    : m_FrameContext(frameContext), m_Renderer(renderer)
{
}

void VkRenderContext::TranslateCommand(const RenderCommand& command)
{
    std::visit([this](auto&& cmd) {
        using T = std::decay_t<decltype(cmd)>;
        if constexpr (std::is_same_v<T, DrawQuadCommand>)
        {
            // TODO: vkCmdBindVertexBuffers + vkCmdBindIndexBuffer + vkCmdDrawIndexed
            (void)cmd;
        }
        else if constexpr (std::is_same_v<T, ClearCommand>)
        {
            // TODO: vkCmdClearAttachments
            (void)cmd;
        }
    }, command);
}

void VkRenderContext::BindPipeline(PipelineHandle handle)
{
    if (m_ActivePipeline == handle) return;
    m_ActivePipeline = handle;
    // TODO: vkCmdBindPipeline
}

void VkRenderContext::BindGeometry(GeometryHandle handle)
{
    if (m_ActiveGeometry == handle) return;
    m_ActiveGeometry = handle;
    // TODO: vkCmdBindVertexBuffers + vkCmdBindIndexBuffer
}

} // namespace Vulkitten
