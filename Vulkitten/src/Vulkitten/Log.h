#pragma once

#include "Vulkitten/Core.h"
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

namespace Vulkitten
{

class VKT_API Log
{
public:
    static void Initialize();

    inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
    inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

private:
    static std::shared_ptr<spdlog::logger> s_CoreLogger;
    static std::shared_ptr<spdlog::logger> s_ClientLogger;
};

}

#define VKT_CORE_ERROR(...)  ::Vulkitten::Log::GetCoreLogger()->error(__VA_ARGS__)
#define VKT_CORE_WARN(...)   ::Vulkitten::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define VKT_CORE_INFO(...)   ::Vulkitten::Log::GetCoreLogger()->info(__VA_ARGS__)
#define VKT_CORE_DEBUG(...)  ::Vulkitten::Log::GetCoreLogger()->debug(__VA_ARGS__)
#define VKT_CORE_TRACE(...)  ::Vulkitten::Log::GetCoreLogger()->trace(__VA_ARGS__)

#define VKT_ERROR(...)  ::Vulkitten::Log::GetClientLogger()->error(__VA_ARGS__)
#define VKT_WARN(...)   ::Vulkitten::Log::GetClientLogger()->warn(__VA_ARGS__)
#define VKT_INFO(...)   ::Vulkitten::Log::GetClientLogger()->info(__VA_ARGS__)
#define VKT_DEBUG(...)  ::Vulkitten::Log::GetClientLogger()->debug(__VA_ARGS__)
#define VKT_TRACE(...)  ::Vulkitten::Log::GetClientLogger()->trace(__VA_ARGS__)
