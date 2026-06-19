#pragma once
#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Core/Window.h"
#include "Vulkitten/Utils/SceneUtil.h"
#include "Vulkitten/Utils/GraphicUtil.h"

namespace Vulkitten {
class RenderContext;

// ============================================================
// GraphicContext — user-visible graphics shell singleton.
//
// Receives existing Window and RenderContext references.
// Does NOT create its own Window — Application owns the Window
// lifecycle and creates it before RenderContext::Init().
// SwapBuffers is handled by EndPass via RenderContext's
// RenderGraph backend context.
// ============================================================
class VKT_API GraphicContext
{
public:
    GraphicContext(Window& window, RenderContext& renderContext);
    ~GraphicContext() = default;

    Window&      GetWindow()      { return m_Window; }
    SceneUtil&   GetSceneUtil()   { return m_SceneUtil; }
    GraphicUtil& GetGraphicUtil()  { return m_GraphicUtil; }

    static GraphicContext& Get() { return *s_Instance; }

private:
    Window&      m_Window;       // Reference to Application-owned Window
    SceneUtil    m_SceneUtil;
    GraphicUtil  m_GraphicUtil;

    static GraphicContext* s_Instance;
};

} // namespace Vulkitten
