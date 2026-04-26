#pragma once

#include "Vulkitten/Core.h"

#include <string>

namespace Vulkitten {

    class VKT_API Shader
    {
    public:
        virtual ~Shader() = default;

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;

        static Ref<Shader> Create(const std::string& vertexSrc, const std::string& fragmentSrc); 
    };

}