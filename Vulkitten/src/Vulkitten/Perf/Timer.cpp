#include "vktpch.h"
#include "Timer.h"

namespace Vulkitten {

    Timer::Timer()
    {
        Reset();
    }

    Timer::Timer(std::function<void(float)> callback)
        : m_Callback(callback)
    {
        Reset();
    }

    Timer::~Timer()
    {
        if (m_Callback) m_Callback(ElapsedMillis());
    }

    void Timer::Reset()
    {
        m_Start = std::chrono::high_resolution_clock::now();
    }

    float Timer::Elapsed()
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_Start).count() * 0.001f * 0.001f * 0.001f;
    }

    float Timer::ElapsedMillis()
    {
        return Elapsed() * 1000.0f;
    }
    
}