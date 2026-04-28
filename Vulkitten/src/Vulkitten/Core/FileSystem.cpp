#include "vktpch.h"
#include "FileSystem.h"

namespace Vulkitten {

    std::map<std::string, std::string> FileSystem::s_Paths;

    void FileSystem::RegisterPath(const std::string& path, const std::string& protocol)
    {
        s_Paths[protocol] = path;
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

        if (s_Paths.find(protocol) == s_Paths.end())
        {
            VKT_CORE_ERROR("Unknown protocol: {0}", protocol);
            return path;
        }

        return s_Paths[protocol] + "/" + filePath;
    }

    bool FileSystem::Exists(const std::string& path)
    {
        std::string resolved = Resolve(path);
        return std::filesystem::exists(resolved);
    }

}