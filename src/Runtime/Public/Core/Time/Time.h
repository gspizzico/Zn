#pragma once

#include <CoreTypes.h>
#include <chrono>

namespace Zn
{
using SteadyClock = std::chrono::steady_clock;
using SystemClock = std::chrono::system_clock;

class Time
{
  public:
    static String Now();

    static String ToString(std::chrono::time_point<SystemClock> timePoint_);

    static uint64 GetTickCount();

    static double Seconds();
};
} // namespace Zn
