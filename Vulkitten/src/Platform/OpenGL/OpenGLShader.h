#pragma once

#include "Vulkitten/Renderer/Shader.h"

#include <string>
#include <glm/glm.hpp>

namespace Vulkitten {

    class VKT_API OpenGLShader : public Shader
    {
    public:
        OpenGLShader(const std::string& filepath);
        OpenGLShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
        virtual ~OpenGLShader() override;

        virtual void Bind() const override;
        virtual void Unbind() const override;

        virtual void SetUniformInt(const std::string& name, int value) override;
        virtual void SetUniformFloat3(const std::string& name, const glm::vec3& value) override;
        virtual void SetUniformFloat4(const std::string& name, const glm::vec4& value) override;
        virtual void SetUniformMat4(const std::string& name, const glm::mat4& value) override;

        virtual const std::string& GetName() const override { return m_Name; }

        void UploadUniformMat4(const std::string& name, const glm::mat4& matrix);
		void UploadUniformFloat4(const std::string& name, const glm::vec4& vector);
		void UploadUniformFloat3(const std::string& name, const glm::vec3& vector);
        void UploadUniformInt(const std::string& name, int value);
    private:
        void Compile(const std::string& vertexSrc, const std::string& fragmentSrc);

        std::string m_Name;
        uint32_t m_RendererID;
    };

}