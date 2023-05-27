#pragma once

#include "Types.h"

namespace Zn
{
struct Guid
{
    uint32 A = 0;
    uint32 B = 0;
    uint32 C = 0;
    uint32 D = 0;

    Guid() = default;

    constexpr Guid(uint32 a, uint32 b, uint32 c, uint32 d)
        : A(a)
        , B(b)
        , C(c)
        , D(d)
    {
    }

    bool operator==(const Guid& other) const
    {
        return A == other.A && B == other.B && C == other.C && D == other.D;
    }

    String ToString() const;

    static Guid Generate();

    // static Guid FromString();

    static constexpr Guid Invalid();
};
} // namespace Zn

namespace std
{
template<>
struct hash<Zn::Guid>
{
    std::size_t operator()(Zn::Guid const& guid) const noexcept
    {
        return hash<Zn::String> {}(guid.ToString());
    }
};
} // namespace std
