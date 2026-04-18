#pragma once

#include "Vulkitten/Core.h"
#include <string>

namespace Vulkitten {

    class Shader
    {
    public:
        Shader(const std::string& vertexSrc, const std::string& fragmentSrc);
        virtual ~Shader();

        virtual void Bind() const;
        virtual void Unbind() const;

    private:
        uint32_t m_RendererID;
    };

}