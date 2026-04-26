#include "vktpch.h"
#include "Shader.h"

#include "Vulkitten/Renderer/Renderer.h"
#include "Vulkitten/Renderer/RendererAPI.h"
#include "Platform/OpenGL/OpenGLShader.h"

namespace Vulkitten {

    Ref<Shader> Shader::Create(const std::string& vertexSrc, const std::string& fragmentSrc)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None:
                VKT_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return CreateRef<OpenGLShader>(vertexSrc, fragmentSrc);
            default:
                VKT_CORE_ASSERT(false, "Unknown RendererAPI!");
                return nullptr;
        }
    }

}