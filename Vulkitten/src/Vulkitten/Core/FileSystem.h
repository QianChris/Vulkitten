#pragma once

#include "Vulkitten/Core.h"

#include <string>
#include <map>
#include <filesystem>

namespace Vulkitten {

    class VKT_API FileSystem
    {
    public:
        static void RegisterPath(const std::string& path, const std::string& protocol);
        static std::string Resolve(const std::string& path);
        static bool Exists(const std::string& path);
    private:
        static std::map<std::string, std::string> s_Paths;
    };

}