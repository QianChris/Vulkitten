#pragma once

#include "Vulkitten/Core.h"

#include <string>
#include <glm/glm.hpp>

namespace Vulkitten {

    class Shader
    {
    public:
        Shader(const std::string& vertexSrc, const std::string& fragmentSrc);
        virtual ~Shader();

        virtual void Bind() const;
        virtual void Unbind() const;

        void UploadUniformMat4(const std::string& name, const glm::mat4& matrix);
    private:
        uint32_t m_RendererID;
    };

}