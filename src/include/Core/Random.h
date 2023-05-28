#pragma once

#include <Core/Types.h>
#include <wyhash/wyhash.h>

namespace Zn
{
inline u64 Random()
{
    static u64 seed = 0;
    return wyrand(&seed);
}

// Returns a random value [min,max)
template<typename T>
inline std::enable_if_t<std::is_arithmetic_v<T>>::type RandomRange(T min_, T max_)
{
    u64    value      = Random();
    double normalized = wy2u01(value);
    return min_ + ((max_ - min_) * normalized);
}
} // namespace Zn
