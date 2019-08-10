#pragma once

#include "Core/HAL/PlatformTypes.h"
#include "Core/Containers/Vector.h"
#include "Core/Name.h"
#include <optional>

namespace Zn
{

    struct LogCategory
    {
        Name    m_Name;
        uint8   m_verbosity;
    };

    class Log
    {
    public:

        Log() = delete;

        //static void DefineLogCategory(const Name& name, uint8 verbosity);

        //static std::optional<LogCategory> GetLogCategory(const Name& name);
    };
}