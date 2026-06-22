#include "vktpch.h"
#include "Framebuffer.h"

#include "Vulkitten/Renderer/Backend/OpenGL/OpenGLFrameBuffer.h"

namespace Vulkitten {

    Ref<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
    {
        return CreateRef<OpenGLFramebuffer>(spec);
    }

}