#pragma once

#include "Core/HAL/PlatformTypes.h"
#include "Core/Containers/Vector.h"
#include "Core/Name.h"
#include "Core/Log/OutputDeviceManager.h"
#include <optional>


namespace Zn
{
    enum class ELogVerbosity : uint8
    {
        Verbose = 0,
        Log     = 1,
        Warning = 2,
        Error   = 3,
        MAX     = 4
    };

    struct LogCategory
    {
        Name            m_Name;
        ELogVerbosity   m_Verbosity;
    };

    class Log
    {
    public:

        Log() = delete;

        static void DefineLogCategory(const Name& name, ELogVerbosity verbosity);

        static bool ModifyVerbosity(const Name& name, ELogVerbosity verbosity);

        template<typename ... Args>
        static void LogMsg(const Name& category, ELogVerbosity verbosity, const char* format, Args&& ... args);
    
        static std::optional<LogCategory> GetLogCategory(const Name& name);

    private:
        static void LogMsgInternal(const Name& category, const char* message);
    };

    struct AutoLogCategory
    {
        AutoLogCategory(Name name, ELogVerbosity verbosity)
        {
            Zn::Log::DefineLogCategory(name, verbosity);
        }
    };

    template<typename ...Args>
    inline void Log::LogMsg(const Name& category, ELogVerbosity verbosity, const char* format, Args&& ... args)
    {
        if (auto LogCategory = GetLogCategory(category); LogCategory.has_value() && verbosity >= LogCategory.value().m_Verbosity)
        {   
            const auto MessageBufferSize = std::snprintf(nullptr, 0, format, std::forward<Args>(args)...);
            
            //#todo [Memory] - use custom allocator
            Vector<char> MessageBuffer(MessageBufferSize + 1); // note +1 for null terminator
            std::snprintf(&MessageBuffer[0], MessageBuffer.size(), format, std::forward<Args>(args)...);

            LogMsgInternal(category, &MessageBuffer[0]);
        }
    }
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