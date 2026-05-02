#include "vktpch.h"
#include "VertexArray.h"

#include "Platform/OpenGL/OpenGLVertexArray.h"
#include "Vulkitten/Renderer/Renderer.h"

#include "Vulkitten/Perf/Instrumentor.h"

namespace Vulkitten {

    Ref<VertexArray> VertexArray::Create()
    {
        VKT_PROFILE_FUNCTION();

        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None:
                VKT_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return CreateRef<OpenGLVertexArray>();
        }

        VKT_CORE_ASSERT(false, "Unknown RendererAPI!");
        return nullptr;
    }

}