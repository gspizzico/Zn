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

        static void DefineLogCategory(const Name& name, uint8 verbosity);

        static bool ModifyVerbosity(const Name& name, uint8 verbosity);

        static std::optional<LogCategory> GetLogCategory(const Name& name);
    };

    struct AutoLogCategory
    {
        AutoLogCategory(Name name, uint8 verbosity)
        {
            Zn::Log::DefineLogCategory(name, verbosity);
        }
    };
}

#define DECLARE_LOG_CATEGORY (Name) \
namespace Zn::LogCategories\
{\
    static Zn::AutoLogCategory LC_##Name;\
}

#define DEFINE_LOG_CATEGORY(Name, Verbosity) \
namespace Zn::LogCategories\
{\
    Zn::AutoLogCategory LC_##Name{#Name, Verbosity};\
}