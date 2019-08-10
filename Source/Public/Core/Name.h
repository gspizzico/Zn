#pragma once
#include <functional>
#include "Core/HAL/PlatformTypes.h"

namespace Zn
{
    struct Name
    {
        Name() = default;

        Name(Zn::String string);

        //Name(const char* chars) : Name(String(chars)) {}

        bool operator==(const Name& other) const { return m_StringCode == other.m_StringCode; }
        
        bool operator<(const Name& other) const { return m_StringCode < other.m_StringCode; }

        operator bool() const { return m_StringCode != 0; }
        
        size_t Value() const { return m_StringCode; }

    private:

        size_t m_StringCode = 0;
    };
}