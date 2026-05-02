#include "vktpch.h"
#include "Texture.h"

#include "Vulkitten/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLTexture.h"

#include "Vulkitten/Perf/Instrumentor.h"

namespace Vulkitten {

    Ref<Texture2D> Texture2D::Create(uint32_t width, uint32_t height)
    {
        VKT_PROFILE_FUNCTION();

        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None:
                VKT_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return CreateRef<OpenGLTexture2D>(width, height);
        }

        VKT_CORE_ASSERT(false, "Unknown RendererAPI!");
        return nullptr;
    }

    Ref<Texture2D> Texture2D::Create(const std::string& path)
    {
        VKT_PROFILE_FUNCTION();

        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None:
                VKT_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return CreateRef<OpenGLTexture2D>(path);
        }

        VKT_CORE_ASSERT(false, "Unknown RendererAPI!");
        return nullptr;
    }

}