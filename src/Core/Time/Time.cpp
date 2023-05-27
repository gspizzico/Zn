#include <Corepch.h>
#include "Time.h"

using namespace Zn;

namespace Zn::Globals
{
static uint64 GStartTime = Time::GetTickCount();
}

inline String Zn::Time::Now()
{
    return ToString(SystemClock::now());
}

String Time::ToString(std::chrono::time_point<SystemClock> time_point)
{
    // Get c time from chrono::system_clock.
    const auto CTime = SystemClock::to_time_t(time_point);

    // Convert to tm struct.
    std::tm TMTime;
    localtime_s(&TMTime, &CTime);

    char
        Buffer[50]; // 50 is an arbitrary number. It's more than enough, considering the format type. If the type changes, change this size.
    auto WrittenSize = std::strftime(&Buffer[0], sizeof(Buffer), "%F:%T", &TMTime);
    check(WrittenSize > 0); // std::strftime returns 0 in case of error.

    return {&Buffer[0], WrittenSize}; // Implicit String constructor.
}
uint64 Time::GetTickCount()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(SteadyClock::now().time_since_epoch()).count();
}
double Time::Seconds()
{
    return static_cast<double>(GetTickCount() - Globals::GStartTime) / 1000.0;
}
