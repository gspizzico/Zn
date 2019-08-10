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
        Zn::String ToHash = string;

        std::transform(ToHash.begin(), ToHash.end(), ToHash.begin(), ::toupper);

        m_StringCode = std::hash<Zn::String>{}(ToHash);

        Zn::Names().try_emplace(m_StringCode, string);
    }
}
