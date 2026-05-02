#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Core/Log.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <string>
#include <thread>
#include <mutex>
#include <sstream>

namespace Vulkitten {

    using FloatingPointMicroseconds = std::chrono::duration<double, std::micro>;

    struct ProfileResult
    {
        std::string Name;

        FloatingPointMicroseconds Start;
        std::chrono::microseconds ElapsedTime;
        std::thread::id ThreadID;
    };

    struct InstrumentationSession
    {
        std::string Name;
    };

    class VKT_API Instrumentor
    {
    public:
        Instrumentor(const Instrumentor&) = delete;
        Instrumentor(Instrumentor&&) = delete;

        void BeginSession(const std::string& name, const std::string& filepath = "results.json");
        void EndSession();
        void WriteProfile(const ProfileResult& result);
        static Instrumentor& Get();
    private:
        Instrumentor();
        ~Instrumentor();

        void WriteHeader();
        void WriteFooter();
        void InternalEndSession();

        std::mutex m_Mutex;
        InstrumentationSession* m_CurrentSession;
        std::ofstream m_OutputStream;
    };

    class InstrumentationTimer
    {
    public:
        InstrumentationTimer(const char* name);
        ~InstrumentationTimer();
        void Stop();
    private:
        const char* m_Name;
        std::chrono::time_point<std::chrono::steady_clock> m_StartTimepoint;
        bool m_Stopped;
    };

    namespace InstrumentorUtils {

        template <size_t N>
        struct ChangeResult
        {
            char Data[N];
        };

        template <size_t N, size_t K>
        constexpr auto CleanupOutputString(const char(&expr)[N], const char(&remove)[K])
        {
            ChangeResult<N> result = {};

            size_t srcIndex = 0;
            size_t dstIndex = 0;
            while (srcIndex < N)
            {
                size_t matchIndex = 0;
                while (matchIndex < K - 1 && srcIndex + matchIndex < N - 1 && expr[srcIndex + matchIndex] == remove[matchIndex])
                    matchIndex++;
                if (matchIndex == K - 1)
                    srcIndex += matchIndex;
                result.Data[dstIndex++] = expr[srcIndex] == '"' ? '\'' : expr[srcIndex];
                srcIndex++;
            }
            return result;
        }
    }
}

#ifndef VKT_PROFILE
#define VKT_PROFILE 0
#endif
#if VKT_PROFILE
    #if defined(__GNUC__) || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) || (defined(__ICC) && (__ICC >= 600)) || defined(__ghs__)
        #define VKT_FUNC_SIG __PRETTY_FUNCTION__
    #elif defined(__DMC__) && (__DMC__ >= 0x810)
        #define VKT_FUNC_SIG __PRETTY_FUNCTION__
    #elif (defined(__FUNCSIG__) || (_MSC_VER))
        #define VKT_FUNC_SIG __FUNCSIG__
    #elif (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 600)) || (defined(__IBMCPP__) && (__IBMCPP__ >= 500))
        #define VKT_FUNC_SIG __FUNCTION__
    #elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)
        #define VKT_FUNC_SIG __FUNC__
    #elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)
        #define VKT_FUNC_SIG __func__
    #elif defined(__cplusplus) && (__cplusplus >= 201103)
        #define VKT_FUNC_SIG __func__
    #else
        #define VKT_FUNC_SIG "VKT_FUNC_SIG unknown!"
    #endif

    #define VKT_PROFILE_BEGIN_SESSION(name, filepath) ::Vulkitten::Instrumentor::Get().BeginSession(name, filepath)
    #define VKT_PROFILE_END_SESSION() ::Vulkitten::Instrumentor::Get().EndSession()
    #define VKT_PROFILE_SCOPE_LINE2(name, line) constexpr auto fixedName##line = ::Vulkitten::InstrumentorUtils::CleanupOutputString(name, "__cdecl ");\
                                                   ::Vulkitten::InstrumentationTimer timer##line(fixedName##line.Data)
    #define VKT_PROFILE_SCOPE_LINE(name, line) VKT_PROFILE_SCOPE_LINE2(name, line)
    #define VKT_PROFILE_SCOPE(name) VKT_PROFILE_SCOPE_LINE(name, __LINE__)
    #define VKT_PROFILE_FUNCTION() VKT_PROFILE_SCOPE(VKT_FUNC_SIG)
    #define VKT_PROFILE_RENDER_FUNCTION() VKT_PROFILE_SCOPE("[RENDER]" VKT_FUNC_SIG)
#else
    #define VKT_PROFILE_BEGIN_SESSION(name, filepath)
    #define VKT_PROFILE_END_SESSION()
    #define VKT_PROFILE_SCOPE(name)
    #define VKT_PROFILE_FUNCTION()
    #define VKT_PROFILE_RENDER_FUNCTION()
#endif
 