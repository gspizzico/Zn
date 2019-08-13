#pragma once

#include "Core/HAL/BasicTypes.h"
#include "Core/Containers/Vector.h"
#include "Core/Name.h"
#include "Core/Log/OutputDeviceManager.h"
#include <optional>

namespace Zn
{
    // Defines the verbosity of the message.
    enum class ELogVerbosity : uint8
    {
        Verbose = 0,
        Log     = 1,
        Warning = 2,
        Error   = 3,
        MAX     = 4
    };

    // Utility struct that wraps a log category.
    struct LogCategory
    {
        Name            m_Name;
        ELogVerbosity   m_Verbosity;
    };

    // Utility class for logging functionalities.
    class Log
    {
    public:
        
        // It provides only static methods, does not need a constructor.
        Log() = delete;

        static void DefineLogCategory(const Name& name, ELogVerbosity verbosity);

        static bool ModifyVerbosity(const Name& name, ELogVerbosity verbosity);

        // Variadic function used to log a message.
        template<typename ... Args>
        static void LogMsg(const Name& category, ELogVerbosity verbosity, const char* format, Args&& ... args);
    
    private:

        // Log Category getter
        static std::optional<LogCategory> GetLogCategory(const Name& name);

        static void LogMsgInternal(const Name& category, ELogVerbosity verbosity, const char* message);

        static const char* ToCString(ELogVerbosity verbosity);
    };

    // Utility struct that autoregisters a log category.
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

            LogMsgInternal(category, verbosity, &MessageBuffer[0]);
        }
    }
}

// Log declarations/definitions macros.
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

#define DECLARE_STATIC_LOG_CATEGORY(Name, Verbosity) \
namespace Zn::GetLogCategories\
{\
    static Zn::AutoLogCategory SLC_##Name{#Name, Verbosity};\
}