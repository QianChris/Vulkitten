#include "vktpch.h"
#include "OpenGLUtil.h"

#include <glad/glad.h>

namespace Vulkitten {

    unsigned int OpenGLUtil::ShaderDataTypeToOpenGLBaseType(ShaderDataType type)
    {
        switch (type)
        {
            case ShaderDataType::Float:   return GL_FLOAT;
            case ShaderDataType::Float2:  return GL_FLOAT;
            case ShaderDataType::Float3:  return GL_FLOAT;
            case ShaderDataType::Float4:  return GL_FLOAT;
            case ShaderDataType::Mat3:    return GL_FLOAT;
            case ShaderDataType::Mat4:    return GL_FLOAT;
            case ShaderDataType::Int:     return GL_INT;
            case ShaderDataType::Int2:    return GL_INT;
            case ShaderDataType::Int3:    return GL_INT;
            case ShaderDataType::Int4:    return GL_INT;
            case ShaderDataType::Uint:    return GL_UNSIGNED_INT;
            case ShaderDataType::Uint2:   return GL_UNSIGNED_INT;
            case ShaderDataType::Uint3:   return GL_UNSIGNED_INT;
            case ShaderDataType::Uint4:   return GL_UNSIGNED_INT;
            case ShaderDataType::Bool:    return GL_BOOL;
        }

        VKT_CORE_ASSERT(false, "Unknown ShaderDataType!");
        return 0;
    }

}