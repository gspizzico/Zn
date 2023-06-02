#pragma once

#include <Core/CoreTypes.h>

namespace Zn
{
struct Guid
{
    uint32 a = 0;
    uint32 b = 0;
    uint32 c = 0;
    uint32 d = 0;

    Guid() = default;

    constexpr Guid(uint32 a_, uint32 b_, uint32 c_, uint32 d_)
        : a(a_)
        , b(b_)
        , c(c_)
        , d(d_)
    {
    }

    bool operator==(const Guid& other_) const
    {
        return a == other_.a && b == other_.b && c == other_.c && d == other_.d;
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
