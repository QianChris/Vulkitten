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
// Created via ClassFactory, receives a RenderContext reference.
// Owns: Window (GLFW), SceneUtil (stub), GraphicUtil (stub).
// SwapBuffers is handled by EndPass via RenderContext's
// RenderGraph backend context — GraphicContext does NOT
// participate in the rendering pipeline.
// ============================================================
class VKT_API GraphicContext
{
public:
    explicit GraphicContext(RenderContext& renderContext);
    ~GraphicContext() = default;

    Window&      GetWindow()      { return *m_Window; }
    SceneUtil&   GetSceneUtil()   { return m_SceneUtil; }
    GraphicUtil& GetGraphicUtil()  { return m_GraphicUtil; }

    static GraphicContext& Get() { return *s_Instance; }

private:
    Scope<Window> m_Window;
    SceneUtil     m_SceneUtil;
    GraphicUtil   m_GraphicUtil;

    static GraphicContext* s_Instance;
};

} // namespace Vulkitten
