#pragma once
#include <functional>
#include "Core/Types.h"

namespace Zn
{
struct Name
{
    Name() = default;

    Name(String string);

    Name(const char* chars)
        : Name(String(chars))
    {
    }

    bool operator==(const Name& other) const
    {
        return m_StringCode == other.m_StringCode;
    }

    bool operator<(const Name& other) const
    {
        return m_StringCode < other.m_StringCode;
    }

    operator bool() const
    {
        return m_StringCode != 0;
    }

    size_t Value() const
    {
        return m_StringCode;
    }

    Zn::String ToString() const;

    const char* const CString() const;

  private:
    size_t m_StringCode = 0;
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
