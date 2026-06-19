#include "vktpch.h"
#include "FileSystem.h"

#include "Vulkitten/Core/Engine.h"

#include <filesystem>

namespace Vulkitten {

    // ============================================================
    // Instance methods (_Impl suffix — Task 5 renames to original)
    // ============================================================

    void FileSystem::RegisterPath_Impl(const std::string& path, const std::string& protocol)
    {
        m_Paths[protocol] = path;
    }

    std::string FileSystem::Resolve_Impl(const std::string& path)
    {
        size_t pos = path.find("://");
        if (pos == std::string::npos)
        {
            return path;
        }

        std::string protocol = path.substr(0, pos);
        std::string filePath = path.substr(pos + 3);

        if (m_Paths.find(protocol) == m_Paths.end())
        {
            VKT_CORE_ERROR("Unknown protocol: {0}", protocol);
            return path;
        }

        return m_Paths[protocol] + "/" + filePath;
    }

    bool FileSystem::Exists_Impl(const std::string& path)
    {
        std::string resolved = Resolve_Impl(path);
        return std::filesystem::exists(resolved);
    }

    // ============================================================
    // Static wrappers (transitional — REMOVED in Task 5)
    // Forward to Engine::Get().GetFileSystem().*_Impl()
    // ============================================================

    void FileSystem::RegisterPath(const std::string& path, const std::string& protocol)
    {
        Engine::Get().GetFileSystem().RegisterPath_Impl(path, protocol);
    }

    std::string FileSystem::Resolve(const std::string& path)
    {
        return Engine::Get().GetFileSystem().Resolve_Impl(path);
    }

    bool FileSystem::Exists(const std::string& path)
    {
        return Engine::Get().GetFileSystem().Exists_Impl(path);
    }

}
