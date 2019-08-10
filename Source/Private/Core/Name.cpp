#include "Core/Name.h"
#include "Core/Containers/Map.h"
#include <algorithm>

namespace Zn
{
    UnorderedMap<size_t, String>& Names()
    {
        static UnorderedMap<size_t, String> s_Names;
        return s_Names;
    }

    Name::Name(Zn::String string)
    {
        std::transform(string.begin(), string.end(), string.begin(), ::toupper);

        m_StringCode = std::hash<Zn::String>{}(string);

        Zn::Names().try_emplace(m_StringCode, string);
    }

    Zn::String Name::ToString() const
    {
        return Zn::Names().at(m_StringCode);
    }
}