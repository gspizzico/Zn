#pragma once
#include <chrono>
#include <ctime>
#include <time.h>
#include "Core/HAL/BasicTypes.h"

namespace Zn
{
    using HighResolutionClock = std::chrono::high_resolution_clock;
    using SystemClock = std::chrono::system_clock;
    
    class Time
    {
    public:

        static String Now() { return ToString(SystemClock::now()); }

        static String ToString(std::chrono::time_point<SystemClock> time_point);
    };
}