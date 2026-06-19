#include "vktpch.h"
#include "FileSystem.h"

#include <filesystem>

namespace Vulkitten {

    void FileSystem::RegisterPath(const std::string& path, const std::string& protocol)
    {
        m_Paths[protocol] = path;
    }

    std::string FileSystem::Resolve(const std::string& path)
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

    bool FileSystem::Exists(const std::string& path)
    {
        std::string resolved = Resolve(path);
        return std::filesystem::exists(resolved);
    }

}
