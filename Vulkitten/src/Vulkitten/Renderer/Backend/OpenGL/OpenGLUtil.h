#pragma once

#include "Vulkitten/Renderer/Buffer.h"

namespace Vulkitten {

    class OpenGLUtil
    {
    public:
        static unsigned int ShaderDataTypeToOpenGLBaseType(ShaderDataType type);
    };

}