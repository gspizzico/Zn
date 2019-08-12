#pragma once
#include <chrono>
#include <ctime>
#include <time.h>
#include "Core/HAL/PlatformTypes.h"

namespace Zn
{
    using HighResolutionClock = std::chrono::high_resolution_clock;
    using SystemClock = std::chrono::system_clock;
    
    class Time
    {
    public:

        static String ToString(std::chrono::time_point<SystemClock> time_point);
    };

    String Time::ToString(std::chrono::time_point<SystemClock> time_point)
    {
        const auto CTime = std::chrono::system_clock::to_time_t(time_point);

        std::tm TMTime;
        localtime_s(&TMTime, &CTime);

        char Buffer[50];
        
        auto WrittenSize = std::strftime(&Buffer[0], sizeof(Buffer), "%F:%T", &TMTime);
        
        _ASSERT(WrittenSize > 0);

        return { &Buffer[0], WrittenSize };
    }
}