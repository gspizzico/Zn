#pragma once
#include <memory>
#include <string>

using int8          = char;
using int16         = short;
using int32         = int;
using int64         = long;
using uint8         = unsigned char;
using uint16        = unsigned short;
using uint32        = unsigned int;
using uint64        = unsigned long;

namespace Zn
{
    using String = std::string;

    template<typename T>
    using SharedPtr     = std::shared_ptr<T>;
}


/* #if PLATFORM_XXX
#include "PlatformXXXTypes.h"
#elif PLATFORM_XXY
...
#endif
*/

#include "Core/Windows/WindowsTypes.h"