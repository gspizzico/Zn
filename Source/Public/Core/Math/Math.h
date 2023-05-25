#pragma once
#include <type_traits>

namespace Zn
{
class Math
{
  public:
    template<typename T, typename = std::enable_if_t<std::is_unsigned<T>::value>>
    static constexpr T Ceil(T number, T multiple)
    {
        return Floor(number + multiple - 1, multiple);
    }

    template<typename T, typename = std::enable_if_t<std::is_unsigned<T>::value>>
    static constexpr T Floor(T number, T multiple)
    {
        return (number / multiple) * multiple;
    }

    template<typename T>
    static constexpr T MapRange(T value, T min, T max, T outMin, T outMax)
    {
        return (value - min) * (outMax - outMin) / (max - min) + outMin;
    }
};
} // namespace Zn
