#include "vktpch.h"
#include "ClassFactory.h"

#include "Vulkitten/Core/Application.h"
#include "Vulkitten/Renderer/RenderContext.h"

#include <random>

namespace Vulkitten {

// ---- UUID generation state (moved from UUID.cpp) ----
static std::random_device s_RandomDevice;
static std::mt19937_64 s_RNGEngine(s_RandomDevice());
static std::uniform_int_distribution<uint64_t> s_UniformDist;

ClassFactory& ClassFactory::Get()
{
    static ClassFactory instance;
    return instance;
}

UUID ClassFactory::GenerateUUID()
{
    return UUID(s_UniformDist(s_RNGEngine));
}

Application& ClassFactory::GetApplication()
{
    return Application::Get();
}

Window& ClassFactory::GetWindow()
{
    return GetApplication().GetWindow();
}

RenderGraph* ClassFactory::GetRenderGraph()
{
    return RenderContext::Get().GetRenderGraph();
}

} // namespace Vulkitten
