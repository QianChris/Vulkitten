#include "vktpch.h"
#include "ShaderManager.h"

#include "Vulkitten/Core/FileSystem.h"
#include "Vulkitten/Core/Engine.h"
#include "Vulkitten/Perf/Instrumentor.h"

#include <fstream>
#include <sstream>

namespace Vulkitten {

// ============================================================
// Construction
// ============================================================

ShaderManager::ShaderManager(FileSystem& fileSystem)
    : m_FileSystem(fileSystem)
{
}

// ============================================================
// Handle Allocation
// ============================================================

uint64_t ShaderManager::AllocateHandle()
{
    uint32_t index;
    if (!m_FreeIndices.empty())
    {
        index = m_FreeIndices.back();
        m_FreeIndices.pop_back();
    }
    else
    {
        index = m_NextIndex++;
    }

    // generation = 0 for now (simplified handle scheme)
    // ShaderManager handles are simpler than GpuResourceManager handles
    // because shaders are currently loaded once and never freed.
    return MakeHandle(index, 0);
}

// ============================================================
// Public API
// ============================================================

uint64_t ShaderManager::LoadShader(const std::string& virtualPath)
{
    VKT_PROFILE_RENDER_FUNCTION();

    // 1. Resolve virtual path → physical path
    std::string resolvedPath = m_FileSystem.Resolve(virtualPath);

    // 2. Read raw source
    std::string source = ReadFileToString(resolvedPath);
    if (source.empty())
    {
        VKT_CORE_ERROR("ShaderManager::LoadShader — failed to read: {0}", virtualPath);
        return UINT64_MAX;
    }

    // 3. Collect include search directories
    std::filesystem::path baseDir = std::filesystem::path(resolvedPath).parent_path();
    std::vector<std::filesystem::path> includeDirs;
    CollectIncludeDirs(includeDirs, baseDir);

    // 4. Resolve #include directives recursively
    std::unordered_set<std::string> guardSet;
    std::string preprocessed = ResolveIncludes(source, includeDirs, guardSet);

    // 5. Store and return handle
    uint64_t handle = AllocateHandle();

    ShaderData data;
    data.VirtualPath        = virtualPath;
    data.ResolvedPath       = resolvedPath;
    data.PreprocessedSource = preprocessed;
    data.IsLoaded           = true;

    m_Shaders[handle] = std::move(data);

    VKT_CORE_INFO("ShaderManager::LoadShader — loaded '{0}'", virtualPath);
    return handle;
}

const ShaderData* ShaderManager::GetShaderData(uint64_t handle) const
{
    auto it = m_Shaders.find(handle);
    return (it != m_Shaders.end()) ? &it->second : nullptr;
}

bool ShaderManager::IsValidHandle(uint64_t handle) const
{
    return m_Shaders.find(handle) != m_Shaders.end();
}

// ============================================================
// Preprocessing Helpers (extracted from OpenGLShader)
// ============================================================

std::string ShaderManager::ReadFileToString(const std::filesystem::path& path)
{
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in.is_open())
        return {};

    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

void ShaderManager::CollectIncludeDirs(
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
        if (parent == dir)
            break;
        dir = parent;
    }
}

std::string ShaderManager::ResolveIncludes(
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
        }
        else
        {
            result += line + "\n";
        }
    }

    return result;
}

} // namespace Vulkitten
