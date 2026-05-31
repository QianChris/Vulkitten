#include "vktpch.h"
#include "OpenGLShader.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>

#include "Vulkitten/Core/FileSystem.h"
#include "Vulkitten/Perf/Instrumentor.h"

#include <filesystem>
#include <vector>
#include <unordered_set>

namespace Vulkitten {

    // ---------------------------------------------------------------------------
    // Include preprocessor for GLSL compute shaders
    // ---------------------------------------------------------------------------
    static std::string ReadFileToString(const std::filesystem::path& path)
    {
        std::ifstream in(path, std::ios::in | std::ios::binary);
        if (!in.is_open()) return {};
        std::stringstream ss;
        ss << in.rdbuf();
        return ss.str();
    }

    static void CollectIncludeDirs(
        std::vector<std::filesystem::path>& outDirs,
        const std::filesystem::path& baseDir)
    {
        // 1. The directory containing the current source file
        outDirs.push_back(baseDir);

        // 2. Walk up from baseDir looking for Vulkitten/src/
        auto dir = baseDir;
        for (int i = 0; i < 10; ++i)
        {
            auto candidate = dir / "Vulkitten";
            if (std::filesystem::exists(candidate / "src"))
            {
                outDirs.push_back(candidate / "src");
                return;
            }
            auto parent = dir.parent_path();
            if (parent == dir) break;
            dir = parent;
        }
    }

    static std::string ResolveIncludes(
        const std::string& source,
        const std::vector<std::filesystem::path>& includeDirs,
        std::unordered_set<std::string>& guardSet)
    {
        std::string result;
        std::istringstream stream(source);
        std::string line;

        while (std::getline(stream, line))
        {
            std::string trimmed = line;
            {
                auto pos = trimmed.find_first_not_of(" \t\r");
                if (pos != std::string::npos)
                    trimmed = trimmed.substr(pos);
                else
                    trimmed.clear();
            }

            // Strip #extension GL_GOOGLE_include_directive (we preprocess includes ourselves)
            if (trimmed.rfind("#extension", 0) == 0 &&
                trimmed.find("GL_GOOGLE_include_directive") != std::string::npos)
            {
                result += "// " + line + "\n";
                continue;
            }

            // Handle #include "..."
            if (trimmed.rfind("#include", 0) == 0)
            {
                auto q1 = trimmed.find('"');
                auto q2 = trimmed.find('"', q1 + 1);
                if (q1 != std::string::npos && q2 != std::string::npos)
                {
                    std::string incPath = trimmed.substr(q1 + 1, q2 - q1 - 1);
                    bool found = false;

                    for (const auto& dir : includeDirs)
                    {
                        auto full = dir / incPath;
                        auto normal = std::filesystem::weakly_canonical(full);
                        if (std::filesystem::exists(normal))
                        {
                            std::string key = normal.string();
                            if (guardSet.count(key))
                            {
                                result += "// already included: " + incPath + "\n";
                            }
                            else
                            {
                                guardSet.insert(key);
                                std::string incSrc = ReadFileToString(normal);
                                if (!incSrc.empty())
                                {
                                    result += "// begin " + incPath + "\n";
                                    result += ResolveIncludes(incSrc, includeDirs, guardSet);
                                    result += "// end " + incPath + "\n";
                                }
                            }
                            found = true;
                            break;
                        }
                    }

                    if (!found)
                    {
                        VKT_CORE_ERROR("GLSL include not found: {0}", incPath);
                        result += line + "\n";
                    }
                }
                else
                {
                    result += line + "\n";
                }
                continue;
            }

            result += line + "\n";
        }

        return result;
    }
    // ---------------------------------------------------------------------------

    OpenGLShader::OpenGLShader(const std::string& filepath)
        : m_Name("unnamed")
    {
        VKT_PROFILE_RENDER_FUNCTION();

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
        VKT_PROFILE_RENDER_FUNCTION();

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

    void OpenGLShader::CompileCompute(const std::string& source)
    {
        VKT_PROFILE_RENDER_FUNCTION();

        GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);

        const char* src = source.c_str();
        glShaderSource(computeShader, 1, &src, nullptr);
        glCompileShader(computeShader);

        int isCompiled = 0;
        glGetShaderiv(computeShader, GL_COMPILE_STATUS, &isCompiled);
        if (isCompiled == GL_FALSE)
        {
            int maxLength = 0;
            glGetShaderiv(computeShader, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<char> infoLog(maxLength);
            glGetShaderInfoLog(computeShader, maxLength, &maxLength, &infoLog[0]);

            glDeleteShader(computeShader);

            VKT_CORE_ERROR("Compute shader compilation failed ({0}):\n{1}", m_Name, infoLog.data());
            VKT_CORE_ASSERT(false, "Compute shader compilation failure!");
            return;
        }

        m_RendererID = glCreateProgram();
        glAttachShader(m_RendererID, computeShader);
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
            glDeleteShader(computeShader);

            VKT_CORE_ERROR("Compute shader linking failed ({0}):\n{1}", m_Name, infoLog.data());
            VKT_CORE_ASSERT(false, "Compute shader linking failure!");
            return;
        }

        glDetachShader(m_RendererID, computeShader);
        glDeleteShader(computeShader);
    }

    OpenGLShader::OpenGLShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc)
        : m_Name(name)
    {
        VKT_PROFILE_RENDER_FUNCTION();

        Compile(vertexSrc, fragmentSrc);
    }

    OpenGLShader::OpenGLShader(const std::string& name, const std::string& filepath, bool isCompute)
        : m_Name(name)
    {
        VKT_PROFILE_RENDER_FUNCTION();

        VKT_CORE_ASSERT(isCompute, "This constructor is only for compute shaders");

        std::string fullPath = FileSystem::Resolve(filepath);

        std::string source = ReadFileToString(fullPath);
        VKT_CORE_ASSERT(!source.empty(), "Failed to read compute shader file: {0}", fullPath);

        std::filesystem::path baseDir = std::filesystem::path(fullPath).parent_path();

        std::vector<std::filesystem::path> includeDirs;
        CollectIncludeDirs(includeDirs, baseDir);

        std::unordered_set<std::string> guardSet;
        std::string finalSource = ResolveIncludes(source, includeDirs, guardSet);

        CompileCompute(finalSource);
    }

    OpenGLShader::~OpenGLShader()
    {
        glDeleteProgram(m_RendererID);
    }

    void OpenGLShader::Bind() const
    {
        VKT_PROFILE_RENDER_FUNCTION();

        glUseProgram(m_RendererID);
    }

    void OpenGLShader::Unbind() const
    {
        VKT_PROFILE_RENDER_FUNCTION();

        glUseProgram(0);
    }

    void OpenGLShader::SetUniformInt(const std::string& name, int value)
    {
        VKT_PROFILE_RENDER_FUNCTION();

        UploadUniformInt(name, value);
    }

    void OpenGLShader::SetUniformFloat(const std::string& name, float value)
    {
        VKT_PROFILE_RENDER_FUNCTION();

        UploadUniformFloat(name, value);
    }

    void OpenGLShader::SetUniformFloat3(const std::string& name, const glm::vec3& value)
    {
        VKT_PROFILE_RENDER_FUNCTION();

        UploadUniformFloat3(name, value);
    }

    void OpenGLShader::SetUniformFloat4(const std::string& name, const glm::vec4& value)
    {
        VKT_PROFILE_RENDER_FUNCTION();

        UploadUniformFloat4(name, value);
    }

    void OpenGLShader::SetUniformMat4(const std::string& name, const glm::mat4& value)
    {
        VKT_PROFILE_RENDER_FUNCTION();

        UploadUniformMat4(name, value);
    }

    // Raw OpenGL uniform upload functions
    void OpenGLShader::UploadUniformMat4(const std::string& name, const glm::mat4& matrix)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
    }
    void OpenGLShader::UploadUniformFloat(const std::string& name, float value)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        glUniform1f(location, value);
    }
    void OpenGLShader::UploadUniformFloat4(const std::string& name, const glm::vec4& vector)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform4f(location, vector.x, vector.y, vector.z, vector.w);
    }
    void OpenGLShader::UploadUniformFloat3(const std::string& name, const glm::vec3& vector)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        glUniform3f(location, vector.x, vector.y, vector.z);
    }
    void OpenGLShader::UploadUniformInt(const std::string& name, int value)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        glUniform1i(location, value);
    }
}