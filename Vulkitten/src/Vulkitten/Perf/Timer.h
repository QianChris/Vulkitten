#pragma once

#include "Vulkitten/Core/Core.h"
#include <chrono>
#include <functional>

namespace Vulkitten {

    class VKT_API Timer
	{
	public:
		Timer();
        Timer(std::function<void(float)> callback);
        ~Timer();
        
		void Timer::Reset();
		float Timer::Elapsed();
		float Timer::ElapsedMillis();

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
        std::function<void(float)> m_Callback;
	};

}