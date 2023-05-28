#include <Misc/Guid.h>
#include <CorePlatform.h>

namespace Zn
{
String Guid::ToString() const
{
    // #todo [Memory] - use custom allocator
    Vector<char> buffer(128ull + 1ull); // note +1 for null terminator

    std::snprintf(&buffer[0], buffer.size(), "%lu-%lu-%lu-%lu", a, b, c, d);

    return String(&buffer[0], buffer.size());
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
