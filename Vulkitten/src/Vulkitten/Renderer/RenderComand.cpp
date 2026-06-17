#include "vktpch.h"
#include "RenderCommand.h"

#include "Platform/OpenGL/OpenGLRendererAPI.h"

namespace Vulkitten {

    RendererAPI* Legacy::RenderCommand::s_RendererAPI = new OpenGLRendererAPI();

}