#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Renderer/FrameContext.h"
#include "Vulkitten/Renderer/RenderGraph/RenderCommand.h"

namespace Vulkitten {

class IRenderer;

using PipelineHandle = uint64_t;
using GeometryHandle = uint64_t;

// ============================================================
// VkRenderContext — Vulkan per-pass command recording context.
//
// Translates RenderCommands into Vulkan vkCmd* calls recorded
// into the active VkCommandBuffer. Manages pipeline and geometry
// binding state to minimize redundant state changes.
// ============================================================
class VkRenderContext
{
public:
    VkRenderContext(FrameContext& frameContext, IRenderer& renderer);

    FrameContext& GetFrameContext() { return m_FrameContext; }
    IRenderer&   GetRenderer()     { return m_Renderer; }

    void SetCommandBuffer(void* cmd) { m_CommandBuffer = cmd; }

    // Translate a single RenderCommand variant to Vulkan drawcalls.
    void TranslateCommand(const RenderCommand& command);

    void BindPipeline(PipelineHandle handle);
    void BindGeometry(GeometryHandle handle);

private:
    FrameContext& m_FrameContext;
    IRenderer&    m_Renderer;
    void*         m_CommandBuffer = nullptr;

    PipelineHandle m_ActivePipeline = 0;
    GeometryHandle m_ActiveGeometry = 0;
};

} // namespace Vulkitten
