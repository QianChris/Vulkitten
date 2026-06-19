#pragma once

#include "Vulkitten/Core/Core.h"

#include <string>
#include <map>

namespace Vulkitten {

    class VKT_API FileSystem
    {
    public:
        FileSystem() = default;

        // ---- Instance API (Task 5 will rename _Impl → original names) ----
        // These implement the actual logic. Static wrappers delegate here.
        void RegisterPath_Impl(const std::string& path, const std::string& protocol);
        std::string Resolve_Impl(const std::string& path);
        bool Exists_Impl(const std::string& path);

        // ---- Static wrappers (transitional — REMOVED in Task 5) ----
        // Keep original names so existing call sites compile unchanged.
        // Forward to Engine::Get().GetFileSystem().RegisterPath_Impl().
        static void RegisterPath(const std::string& path, const std::string& protocol);
        static std::string Resolve(const std::string& path);
        static bool Exists(const std::string& path);

    private:
        std::map<std::string, std::string> m_Paths;
    };

}