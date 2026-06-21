#include "vktpch.h"
#include "OpenGLShader.h"

#include "Vulkitten/Core/Engine.h"
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <vector>

#include "Vulkitten/Core/FileSystem.h"
#include "Vulkitten/Perf/Instrumentor.h"

namespace Vulkitten {

    // ============================================================
    // SPIR-V binary reader
    // ============================================================
    static std::vector<uint32_t> ReadSpvFile(const std::string& path)
    {
        std::ifstream in(path, std::ios::binary | std::ios::ate);
        if (!in.is_open())
            return {};

        size_t fileSize = static_cast<size_t>(in.tellg());
        in.seekg(0, std::ios::beg);

        std::vector<uint32_t> spv(fileSize / sizeof(uint32_t));
        in.read(reinterpret_cast<char*>(spv.data()), fileSize);
        return spv;
    }

    // ============================================================
    // Constructors
    // ============================================================

    OpenGLShader::OpenGLShader(const std::string& filepath)
        : m_Name("unnamed")
    {
        VKT_PROFILE_RENDER_FUNCTION();

        std::string resolved = Engine::Get().GetFileSystem().Resolve(filepath);
        std::string vertPath = resolved + ".vert.spv";
        std::string fragPath = resolved + ".frag.spv";

        CompileFromSpv(vertPath, fragPath);
    }

    OpenGLShader::OpenGLShader(const std::string& name, const std::string& filepath, bool isCompute)
        : m_Name(name)
    {
        VKT_PROFILE_RENDER_FUNCTION();

        if (isCompute)
        {
            std::string resolved = Engine::Get().GetFileSystem().Resolve(filepath);
            CompileComputeFromSpv(resolved + ".spv");
        }
        else
        {
            std::string resolved = Engine::Get().GetFileSystem().Resolve(filepath);
            CompileFromSpv(resolved + ".vert.spv", resolved + ".frag.spv");
        }
    }

    OpenGLShader::~OpenGLShader()
    {
        glDeleteProgram(m_RendererID);
    }

    // ============================================================
    // SPIR-V Compilation (replaces GLSL Compile/CompileCompute)
    // ============================================================

    void OpenGLShader::CompileFromSpv(const std::string& vertSpvPath, const std::string& fragSpvPath)
    {
        VKT_PROFILE_RENDER_FUNCTION();

        auto vertSpv = ReadSpvFile(vertSpvPath);
        if (vertSpv.empty())
        {
            VKT_CORE_ERROR("Failed to read vertex SPIR-V: {0}", vertSpvPath);
            return;
        }

        auto fragSpv = ReadSpvFile(fragSpvPath);
        if (fragSpv.empty())
        {
            VKT_CORE_ERROR("Failed to read fragment SPIR-V: {0}", fragSpvPath);
            return;
        }

        // Create vertex shader from SPIR-V
        GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
        const GLchar* vertSrc = reinterpret_cast<const GLchar*>(vertSpv.data());
        GLint vertLen = static_cast<GLint>(vertSpv.size() * sizeof(uint32_t));
        glShaderBinary(1, &vertShader, GL_SHADER_BINARY_FORMAT_SPIR_V, vertSrc, vertLen);
        glSpecializeShader(vertShader, "main", 0, nullptr, nullptr);

        GLint compiled = 0;
        glGetShaderiv(vertShader, GL_COMPILE_STATUS, &compiled);
        if (!compiled)
        {
            GLint maxLen = 0;
            glGetShaderiv(vertShader, GL_INFO_LOG_LENGTH, &maxLen);
            std::vector<GLchar> log(maxLen);
            glGetShaderInfoLog(vertShader, maxLen, &maxLen, log.data());
            VKT_CORE_ERROR("Vertex SPIR-V specialization failed: {0}", log.data());
            glDeleteShader(vertShader);
            return;
        }

        // Create fragment shader from SPIR-V
        GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
        const GLchar* fragSrc = reinterpret_cast<const GLchar*>(fragSpv.data());
        GLint fragLen = static_cast<GLint>(fragSpv.size() * sizeof(uint32_t));
        glShaderBinary(1, &fragShader, GL_SHADER_BINARY_FORMAT_SPIR_V, fragSrc, fragLen);
        glSpecializeShader(fragShader, "main", 0, nullptr, nullptr);

        glGetShaderiv(fragShader, GL_COMPILE_STATUS, &compiled);
        if (!compiled)
        {
            GLint maxLen = 0;
            glGetShaderiv(fragShader, GL_INFO_LOG_LENGTH, &maxLen);
            std::vector<GLchar> log(maxLen);
            glGetShaderInfoLog(fragShader, maxLen, &maxLen, log.data());
            VKT_CORE_ERROR("Fragment SPIR-V specialization failed: {0}", log.data());
            glDeleteShader(vertShader);
            glDeleteShader(fragShader);
            return;
        }

        // Link program
        m_RendererID = glCreateProgram();
        glAttachShader(m_RendererID, vertShader);
        glAttachShader(m_RendererID, fragShader);
        glLinkProgram(m_RendererID);

        GLint linked = 0;
        glGetProgramiv(m_RendererID, GL_LINK_STATUS, &linked);
        if (!linked)
        {
            GLint maxLen = 0;
            glGetProgramiv(m_RendererID, GL_INFO_LOG_LENGTH, &maxLen);
            std::vector<GLchar> log(maxLen);
            glGetProgramInfoLog(m_RendererID, maxLen, &maxLen, log.data());
            VKT_CORE_ERROR("Shader link failed: {0}", log.data());
            glDeleteProgram(m_RendererID);
            m_RendererID = 0;
        }

        glDetachShader(m_RendererID, vertShader);
        glDetachShader(m_RendererID, fragShader);
        glDeleteShader(vertShader);
        glDeleteShader(fragShader);
    }

    void OpenGLShader::CompileComputeFromSpv(const std::string& spvPath)
    {
        VKT_PROFILE_RENDER_FUNCTION();

        auto spv = ReadSpvFile(spvPath);
        if (spv.empty())
        {
            VKT_CORE_ERROR("Failed to read compute SPIR-V: {0}", spvPath);
            return;
        }

        GLuint compShader = glCreateShader(GL_COMPUTE_SHADER);
        const GLchar* src = reinterpret_cast<const GLchar*>(spv.data());
        GLint len = static_cast<GLint>(spv.size() * sizeof(uint32_t));
        glShaderBinary(1, &compShader, GL_SHADER_BINARY_FORMAT_SPIR_V, src, len);
        glSpecializeShader(compShader, "main", 0, nullptr, nullptr);

        GLint compiled = 0;
        glGetShaderiv(compShader, GL_COMPILE_STATUS, &compiled);
        if (!compiled)
        {
            GLint maxLen = 0;
            glGetShaderiv(compShader, GL_INFO_LOG_LENGTH, &maxLen);
            std::vector<GLchar> log(maxLen);
            glGetShaderInfoLog(compShader, maxLen, &maxLen, log.data());
            VKT_CORE_ERROR("Compute SPIR-V specialization failed: {0}", log.data());
            glDeleteShader(compShader);
            return;
        }

        m_RendererID = glCreateProgram();
        glAttachShader(m_RendererID, compShader);
        glLinkProgram(m_RendererID);

        GLint linked = 0;
        glGetProgramiv(m_RendererID, GL_LINK_STATUS, &linked);
        if (!linked)
        {
            GLint maxLen = 0;
            glGetProgramiv(m_RendererID, GL_INFO_LOG_LENGTH, &maxLen);
            std::vector<GLchar> log(maxLen);
            glGetProgramInfoLog(m_RendererID, maxLen, &maxLen, log.data());
            VKT_CORE_ERROR("Compute SPIR-V link failed: {0}", log.data());
            glDeleteProgram(m_RendererID);
            m_RendererID = 0;
        }

        glDetachShader(m_RendererID, compShader);
        glDeleteShader(compShader);
    }

    // ============================================================
    // Bind / Unbind
    // ============================================================

    void OpenGLShader::Bind() const
    {
        glUseProgram(m_RendererID);
    }

    void OpenGLShader::Unbind() const
    {
        glUseProgram(0);
    }

    // ============================================================
    // Uniform uploads
    // ============================================================

    void OpenGLShader::SetUniformInt(const std::string& name, int value)
    {
        GLint loc = glGetUniformLocation(m_RendererID, name.c_str());
        if (loc != -1) glUniform1i(loc, value);
    }

    void OpenGLShader::SetUniformFloat(const std::string& name, float value)
    {
        GLint loc = glGetUniformLocation(m_RendererID, name.c_str());
        if (loc != -1) glUniform1f(loc, value);
    }

    void OpenGLShader::SetUniformFloat3(const std::string& name, const glm::vec3& value)
    {
        GLint loc = glGetUniformLocation(m_RendererID, name.c_str());
        if (loc != -1) glUniform3f(loc, value.x, value.y, value.z);
    }

    void OpenGLShader::SetUniformFloat4(const std::string& name, const glm::vec4& value)
    {
        GLint loc = glGetUniformLocation(m_RendererID, name.c_str());
        if (loc != -1) glUniform4f(loc, value.x, value.y, value.z, value.w);
    }

    void OpenGLShader::SetUniformMat4(const std::string& name, const glm::mat4& value)
    {
        GLint loc = glGetUniformLocation(m_RendererID, name.c_str());
        if (loc != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(value));
    }

} // namespace Vulkitten
