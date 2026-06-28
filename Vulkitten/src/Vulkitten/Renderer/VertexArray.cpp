#include "vktpch.h"
#include "VertexArray.h"

#include "Vulkitten/Renderer/Backend/OpenGL/OpenGLVertexArray.h"

#include "Vulkitten/Perf/Instrumentor.h"

namespace Vulkitten {

    Ref<VertexArray> VertexArray::Create()
    {
        VKT_PROFILE_FUNCTION();
        return CreateRef<OpenGLVertexArray>();
    }

}