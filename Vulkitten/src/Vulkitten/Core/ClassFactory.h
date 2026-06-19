#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Core/UUID.h"

namespace Vulkitten {

class Application;
class Window;
class RenderGraph;

// ============================================================
// ClassFactory — the single centralized singleton for the engine.
// All subsystem access and object creation routes through here.
// ============================================================
class VKT_API ClassFactory
{
public:
    static ClassFactory& Get();

    // ---- UUID Generation ----
    UUID GenerateUUID();

    // ---- Core Subsystem Access ----
    Application& GetApplication();
    Window& GetWindow();

    // ---- Renderer Subsystem Access ----
    RenderGraph* GetRenderGraph();

    // Prevent copies
    ClassFactory(const ClassFactory&) = delete;
    ClassFactory& operator=(const ClassFactory&) = delete;

private:
    ClassFactory() = default;
    ~ClassFactory() = default;
};

} // namespace Vulkitten
