#include "vktpch.h"
#include "OpenGLShader.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>

#include <nlohmann/json.hpp>
#include "Vulkitten/Core/FileSystem.h"

namespace Vulkitten {

    OpenGLShader::OpenGLShader(const std::string& filepath)
        : m_Name("unnamed")
    {
        std::string fullPath = FileSystem::Resolve(filepath);

        std::ifstream in(fullPath);
        if (!in.is_open())
        {
            VKT_CORE_ERROR("Failed to open shader file: {0}", fullPath);
            VKT_CORE_ASSERT(false, "Shader file not found");
            return;
        }

        std::stringstream ss;
        ss << in.rdbuf();
        std::string jsonStr = ss.str();
        in.close();

        auto j = nlohmann::json::parse(jsonStr);

        std::string vertPath = FileSystem::Resolve(j["vertex"].get<std::string>());
        std::string fragPath = FileSystem::Resolve(j["fragment"].get<std::string>());

        std::ifstream vertIn(vertPath);
        std::stringstream vertSs;
        vertSs << vertIn.rdbuf();
        std::string vertexSrc = vertSs.str();
        vertIn.close();

        std::ifstream fragIn(fragPath);
        std::stringstream fragSs;
        fragSs << fragIn.rdbuf();
        std::string fragmentSrc = fragSs.str();
        fragIn.close();

        Compile(vertexSrc, fragmentSrc);
    }

    void OpenGLShader::Compile(const std::string& vertexSrc, const std::string& fragmentSrc)
    {
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);

        const char* source = vertexSrc.c_str();
        glShaderSource(vertexShader, 1, &source, nullptr);

        glCompileShader(vertexShader);

        int isCompiled = 0;
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &isCompiled);
        if (isCompiled == GL_FALSE)
        {
            int maxLength = 0;
            glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<char> infoLog(maxLength);
            glGetShaderInfoLog(vertexShader, maxLength, &maxLength, &infoLog[0]);

            glDeleteShader(vertexShader);

            VKT_CORE_ERROR("Vertex shader compilation failed: {0}", infoLog.data());
            VKT_CORE_ASSERT(false, "Vertex shader compilation failure!");
        }
        
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        source = fragmentSrc.c_str();
        glShaderSource(fragmentShader, 1, &source, nullptr);
        glCompileShader(fragmentShader);
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &isCompiled);
        if (isCompiled == GL_FALSE)
        {
            int maxLength = 0;
            glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<char> infoLog(maxLength);
            glGetShaderInfoLog(fragmentShader, maxLength, &maxLength, &infoLog[0]);

            glDeleteShader(fragmentShader);
            glDeleteShader(vertexShader);

            VKT_CORE_ERROR("Fragment shader compilation failed: {0}", infoLog.data());
            VKT_CORE_ASSERT(false, "Fragment shader compilation failure!");
        }

        m_RendererID = glCreateProgram();
        glAttachShader(m_RendererID, vertexShader);
        glAttachShader(m_RendererID, fragmentShader);
        glLinkProgram(m_RendererID);
        int isLinked = 0;
        glGetProgramiv(m_RendererID, GL_LINK_STATUS, (int*)&isLinked);
        if (isLinked == GL_FALSE)
        {
            int maxLength = 0;
            glGetProgramiv(m_RendererID, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<char> infoLog(maxLength);
            glGetProgramInfoLog(m_RendererID, maxLength, &maxLength, &infoLog[0]);

            glDeleteProgram(m_RendererID);
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);

            VKT_CORE_ERROR("Shader linking failed: {0}", infoLog.data());
            VKT_CORE_ASSERT(false, "Shader linking failure!");
        }

        glDetachShader(m_RendererID, vertexShader);
        glDetachShader(m_RendererID, fragmentShader);
    }

    OpenGLShader::OpenGLShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc)
        : m_Name(name)
    {
        Compile(vertexSrc, fragmentSrc);
    }

    OpenGLShader::~OpenGLShader()
    {
        glDeleteProgram(m_RendererID);
    }

    void OpenGLShader::Bind() const
    {
        glUseProgram(m_RendererID);
    }

    void OpenGLShader::Unbind() const
    {
        glUseProgram(0);
    }

    const std::string& OpenGLShader::GetName() const
    {
        return m_Name;
    }

    void OpenGLShader::UploadUniformMat4(const std::string& name, const glm::mat4& matrix)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
    }
    void OpenGLShader::UploadUniformFloat4(const std::string& name, const glm::vec4& vector)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform4f(location, vector.x, vector.y, vector.z, vector.w);
    }
    void OpenGLShader::UploadUniformInt(const std::string& name, int value)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        glUniform1i(location, value);
    }
}