#include "vktpch.h"
#include "RenderContext.h"

#include "Vulkitten/Renderer/IRenderer.h"
#include "Vulkitten/Renderer/IGpuResourceManager.h"

namespace Vulkitten {

RenderContext::RenderContext(FrameContext& frameContext, IRenderer& renderer)
    : m_FrameContext(frameContext)
    , m_Renderer(renderer)
{
}

void RenderContext::TranslateCommand(const RenderCommand& command)
{
    std::visit([this](auto&& cmd) {
        using T = std::decay_t<decltype(cmd)>;

        if constexpr (std::is_same_v<T, DrawQuadCommand>)
        {
            // TODO Task 6: translate to backend drawcall via RendererAPI
            (void)cmd;
        }
        else if constexpr (std::is_same_v<T, ClearCommand>)
        {
            // TODO Task 6: translate to backend clear via RendererAPI
            (void)cmd;
        }
    }, command);
}

void RenderContext::BindPipeline(PipelineHandle handle)
{
    if (m_ActivePipeline == handle) return;
    m_ActivePipeline = handle;

    // TODO Task 6: bind pipeline via backend
}

void RenderContext::BindGeometry(GeometryHandle handle)
{
    if (m_ActiveGeometry == handle) return;
    m_ActiveGeometry = handle;

    // TODO Task 6: bind geometry via backend
}

} // namespace Vulkitten
