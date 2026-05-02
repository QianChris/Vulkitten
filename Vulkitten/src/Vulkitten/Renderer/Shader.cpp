#include "vktpch.h"
#include "Shader.h"

#include "Vulkitten/Renderer/Renderer.h"
#include "Vulkitten/Renderer/RendererAPI.h"
#include "Platform/OpenGL/OpenGLShader.h"

#include "Vulkitten/Perf/Instrumentor.h"

namespace Vulkitten {

    Ref<Shader> Shader::Create(const std::string& filepath)
    {
        VKT_PROFILE_FUNCTION();

        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None:
                VKT_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return CreateRef<OpenGLShader>(filepath);
            default:
                VKT_CORE_ASSERT(false, "Unknown RendererAPI!");
                return nullptr;
        }
    }

    Ref<Shader> Shader::Create(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc)
    {
        VKT_PROFILE_FUNCTION();

        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None:
                VKT_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return CreateRef<OpenGLShader>(name, vertexSrc, fragmentSrc);
            default:
                VKT_CORE_ASSERT(false, "Unknown RendererAPI!");
                return nullptr;
        }
    }

    void ShaderLibrary::Add(const std::string& name, const Ref<Shader>& shader)
    {
        VKT_PROFILE_FUNCTION();

        VKT_CORE_ASSERT(!Exists(name), "Shader already exists!");
        m_Shaders[name] = shader;
    }

    void ShaderLibrary::Add(const Ref<Shader>& shader)
    {
        VKT_PROFILE_FUNCTION();

        Add(shader->GetName(), shader);
    }

    void ShaderLibrary::Load(const std::string& filepath)
    {
        VKT_PROFILE_FUNCTION();

        auto shader = Shader::Create(filepath);

        // Get name from filepath
        auto begin = filepath.find_last_of("/\\") + 1;
        if (begin == std::string::npos) begin = 0;
        auto end = filepath.find_last_of('.');
        if (end == std::string::npos) end = filepath.size();
        auto name = filepath.substr(begin, end - begin);

        Add(name, shader);
    }

    Ref<Shader> ShaderLibrary::Load(const std::string& name, const std::string& filepath)
    {
        VKT_PROFILE_FUNCTION();

        auto shader = Shader::Create(filepath);
        Add(name, shader);
        return shader;
    }

    Ref<Shader> ShaderLibrary::Get(const std::string& name)
    {
        VKT_PROFILE_FUNCTION();

        VKT_CORE_ASSERT(Exists(name), "Shader not found!");
        return m_Shaders[name];
    }

    bool ShaderLibrary::Exists(const std::string& name) const
    {
        VKT_PROFILE_FUNCTION();

        return m_Shaders.find(name) != m_Shaders.end();
    }

}