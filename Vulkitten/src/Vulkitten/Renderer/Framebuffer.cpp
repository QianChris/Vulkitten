#include "vktpch.h"
#include "Framebuffer.h"

#include "Vulkitten/Renderer/Renderer.h"

#include "Vulkitten/Renderer/Backend/OpenGL/OpenGLFrameBuffer.h"

namespace Vulkitten {

    Ref<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
    {
        switch (Renderer::GetAPI()) {
            case RendererAPI::API::None:
                VKT_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return CreateRef<OpenGLFramebuffer>(spec);
        }

        VKT_CORE_ASSERT(false, "Unknown RendererAPI!");
        return nullptr;
    }

}