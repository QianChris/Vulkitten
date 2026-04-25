#pragma once

#include "Vulkitten/Renderer/Shader.h"

#include <string>
#include <glm/glm.hpp>

namespace Vulkitten {

    class VKT_API OpenGLShader : public Shader
    {
    public:
        OpenGLShader(const std::string& vertexSrc, const std::string& fragmentSrc);
        virtual ~OpenGLShader() override;

        virtual void Bind() const override;
        virtual void Unbind() const override;

        void UploadUniformMat4(const std::string& name, const glm::mat4& matrix);
		void UploadUniformFloat4(const std::string& name, const glm::vec4& vector);
    private:
        uint32_t m_RendererID;
    };

}