#pragma once

#include "Vulkitten/Core/Core.h"

#include <string>
#include <map>

namespace Vulkitten {

    class VKT_API FileSystem
    {
    public:
        FileSystem() = default;

        // Register a virtual path mapping: protocol → physical path.
        void RegisterPath(const std::string& path, const std::string& protocol);

        // Resolve a virtual path to its physical equivalent.
        std::string Resolve(const std::string& path);

        // Check whether a file exists at the given virtual or physical path.
        bool Exists(const std::string& path);

    private:
        std::map<std::string, std::string> m_Paths;
    };

}