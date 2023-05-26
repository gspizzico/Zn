#include <Corepch.h>
#include "HAL/Guid.h"
#include "HAL/Misc.h"

namespace Zn
{
const Guid Guid::kNone = Guid {
    .A = 0,
    .B = 0,
    .C = 0,
    .D = 0,
};

String Guid::ToString() const
{
    // #todo [Memory] - use custom allocator
    Vector<char> MessageBuffer(128ull + 1ull); // note +1 for null terminator

    std::snprintf(&MessageBuffer[0], MessageBuffer.size(), "%lu-%lu-%lu-%lu", A, B, C, D);

    return String(&MessageBuffer[0], MessageBuffer.size());
}

Guid Guid::Generate()
{
    return Misc::GenerateGuid();
}
} // namespace Zn
