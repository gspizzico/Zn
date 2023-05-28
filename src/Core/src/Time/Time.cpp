#include <Time/Time.h>
#include <CoreAssert.h>

using namespace Zn;

namespace
{
static uint64 GStartTime = Time::GetTickCount();
}

inline String Zn::Time::Now()
{
    return ToString(SystemClock::now());
}

String Time::ToString(std::chrono::time_point<SystemClock> timePoint_)
{
    // Get c time from chrono::system_clock.
    const auto cTime = SystemClock::to_time_t(timePoint_);

    // Convert to tm struct.
    std::tm tmTime;
    localtime_s(&tmTime, &cTime);

    // 50 is an arbitrary number. It's more than enough, considering the format type. If the type changes, change this size.
    char buffer[50];
    auto writtenSize = std::strftime(&buffer[0], sizeof(buffer), "%F:%T", &tmTime);
    check(writtenSize > 0); // std::strftime returns 0 in case of error.

    return {&buffer[0], writtenSize}; // Implicit String constructor.
}
uint64 Time::GetTickCount()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(SteadyClock::now().time_since_epoch()).count();
}
double Time::Seconds()
{
    return static_cast<double>(GetTickCount() - GStartTime) / 1000.0;
}
