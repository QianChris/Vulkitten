#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Renderer/IRenderer.h"

namespace Vulkitten {

enum class RendererBackend : uint8_t
{
    OpenGL,
    Vulkan,
};

// ============================================================
// RendererFactory - creates the correct IRenderer backend.
//
// Encapsulates backend selection so Application.cpp contains
// no #ifdef OPENGL / #ifdef VULKAN guards. The factory .cpp
// includes the concrete backend headers.
// ============================================================
class RendererFactory
{
public:
    static Scope<IRenderer> Create(RendererBackend backend, const RendererConfig& config);
};

} // namespace Vulkitten
