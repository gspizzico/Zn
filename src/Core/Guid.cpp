#include <Corepch.h>
#include "Guid.h"

namespace Zn
{
String Guid::ToString() const
{
    // #todo [Memory] - use custom allocator
    Vector<char> MessageBuffer(128ull + 1ull); // note +1 for null terminator

    std::snprintf(&MessageBuffer[0], MessageBuffer.size(), "%lu-%lu-%lu-%lu", A, B, C, D);

    return String(&MessageBuffer[0], MessageBuffer.size());
}

Guid Guid::Generate()
{
    return PlatformMisc::GenerateGuid();
}
constexpr Guid Guid::Invalid()
{
    return Guid(0, 0, 0, 0);
}
} // namespace Zn
