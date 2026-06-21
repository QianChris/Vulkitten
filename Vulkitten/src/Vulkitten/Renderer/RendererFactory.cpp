#include "vktpch.h"
#include "RendererFactory.h"

#include "Vulkitten/Renderer/Backend/OpenGL/OpenGLRenderer.h"
#include "Vulkitten/Renderer/Backend/Vulkan/VkRenderer.h"

namespace Vulkitten {

Scope<IRenderer> RendererFactory::Create(RendererBackend backend, const RendererConfig& config)
{
    switch (backend)
    {
        case RendererBackend::Vulkan:
            return CreateScope<VkRenderer>(config);
        case RendererBackend::OpenGL:
        default:
            return CreateScope<OpenGLRenderer>(config);
    }
}

} // namespace Vulkitten
