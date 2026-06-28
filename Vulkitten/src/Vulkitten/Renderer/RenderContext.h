#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Renderer/FrameContext.h"
#include "Vulkitten/Renderer/RenderGraph/RenderCommand.h"

#include <cstdint>
#include <unordered_map>

namespace Vulkitten {

class IRenderer;

// ============================================================
// PipelineHandle - lightweight handle for cached pipeline state.
//
// Points to a compiled pipeline (VkPipeline / GL program) stored
// in IGpuResourceManager. Used by RenderContext to bind the
// correct pipeline before issuing drawcalls.
// ============================================================
using PipelineHandle = uint64_t;

// ============================================================
// GeometryHandle - lightweight handle for geometry data.
//
// Points to vertex + index buffer resources in IGpuResourceManager.
// ============================================================
using GeometryHandle = uint64_t;

// ============================================================
// RenderContext - per-pass command translation context.
//
// Created at the start of each render pass. Translates abstract
// RenderCommands into concrete API drawcalls (OpenGL glDraw*
// or Vulkan vkCmdDraw*). Maintains temporary state mappings
// (active pipeline, bound geometry) to avoid redundant binds.
//
// For OpenGL: immediately executes drawcalls via RendererAPI.
// For Vulkan: records commands into the active VkCommandBuffer.
// ============================================================
class RenderContext
{
public:
    RenderContext(FrameContext& frameContext, IRenderer& renderer);
    ~RenderContext() = default;

    // ---- Accessors ----

    FrameContext& GetFrameContext() { return m_FrameContext; }
    IRenderer&   GetRenderer()     { return m_Renderer; }

    // ---- Command Translation ----

    // Translate a single RenderCommand variant into API drawcalls.
    // Dispatches on the variant type and calls the appropriate
    // backend-specific draw method.
    void TranslateCommand(const RenderCommand& command);

    // ---- Pipeline & Geometry State ----

    // Bind a pipeline for subsequent drawcalls. Cached to avoid
    // redundant binds (no-op if handle matches active pipeline).
    void BindPipeline(PipelineHandle handle);

    // Bind geometry (vertex + index buffers) for subsequent draws.
    void BindGeometry(GeometryHandle handle);

private:
    FrameContext& m_FrameContext;
    IRenderer&    m_Renderer;

    // ---- Temporary State Cache ----
    PipelineHandle m_ActivePipeline = 0;
    GeometryHandle m_ActiveGeometry = 0;
};

} // namespace Vulkitten
