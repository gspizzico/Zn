#include <Core/Corepch.h>
#include <algorithm>

namespace Zn
{
UnorderedMap<size_t, String>& Names()
{
    static UnorderedMap<size_t, String> names;
    return names;
}

Name::Name(Zn::String string)
{
    std::transform(string.begin(), string.end(), string.begin(), ::toupper);

    stringKey = std::hash<Zn::String> {}(string);

    Zn::Names().try_emplace(stringKey, string);
}

Zn::String Name::ToString() const
{
    if (*this == NO_NAME)
        return "";

    return Zn::Names().at(stringKey);
}

cstring const Name::CString() const
{
    if (*this == NO_NAME)
        return "";

    return Zn::Names().at(stringKey).c_str();
}
} // namespace Zn
