#pragma once

#include <Core/CoreTypes.h>
#include <functional>

namespace Zn
{
struct Name
{
    Name() = default;

    Name(String string_);

    Name(cstring cstring_)
        : Name(String(cstring_))
    {
    }

    bool operator==(const Name& other_) const
    {
        return stringKey == other_.stringKey;
    }

    bool operator<(const Name& other) const
    {
        return stringKey < other.stringKey;
    }

    operator bool() const
    {
        return stringKey != 0;
    }

    size_t Value() const
    {
        return stringKey;
    }

    Zn::String ToString() const;

    cstring const CString() const;

  private:
    sizet stringKey = 0;
};

static const Name NO_NAME;
} // namespace Zn

namespace std
{
template<>
struct hash<Zn::Name>
{
    size_t operator()(const Zn::Name& name) const noexcept
    {
        return name.Value();
    }
};
} // namespace std
