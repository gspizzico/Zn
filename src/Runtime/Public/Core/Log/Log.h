#pragma once

#include <Core/CoreTypes.h>
#include <Core/Misc/Name.h>
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
  public:
    cstring name;

    ELogVerbosity verbosity;

    bool IsSuppressed(ELogVerbosity verbosity_) const
    {
        return verbosity > verbosity_;
    }
};

struct LogCategoryHandle
{
    static constexpr uint16 kInvalidHandle = u16_max;

    uint16 handle = kInvalidHandle;
};

typedef void (*PFN_LogMessageCallback)(cstring message_);

// Utility class for logging functionalities.
class Log
{
  public:
    // It provides only static methods, does not need a constructor.
    Log() = delete;

    static LogCategoryHandle AddLogCategory(const LogCategory& category_);

    static bool ModifyVerbosity(cstring name_, ELogVerbosity verbosity_);

    // Variadic function used to log a message.
    template<typename... Args>
    static void LogMsg(LogCategoryHandle handle_, ELogVerbosity verbosity_, const char* format_, Args&&... args_);

    static void SetLogMessageCallback(PFN_LogMessageCallback callback_);

    static const LogCategory& GetLogCategory(LogCategoryHandle handle_);

  private:
    // Log Category getter
    static LogCategory* GetLogCategory(cstring name_);

    static void LogMsgInternal(LogCategoryHandle handle_, ELogVerbosity verbosity_, const char* message_);

    static cstring ToCString(ELogVerbosity verbosity_);
};

// Utility struct that autoregisters a log category.
struct AutoLogCategory
{
    AutoLogCategory() = default;

    AutoLogCategory(cstring name_, ELogVerbosity verbosity_)
    {
        handle = Zn::Log::AddLogCategory(LogCategory {
            .name      = name_,
            .verbosity = verbosity_,
        });
    }

    LogCategoryHandle handle {};

    bool IsSuppressed(ELogVerbosity verbosity_) const
    {
        return Log::GetLogCategory(handle).IsSuppressed(verbosity_);
    }
};

template<typename... Args>
inline void Log::LogMsg(LogCategoryHandle handle_, ELogVerbosity verbosity_, const char* format_, Args&&... args_)
{
    char buffer[512];

    std::snprintf(buffer, sizeof(buffer), format_, std::forward<Args>(args_)...);

    LogMsgInternal(handle_, verbosity_, &buffer[0]);
}
} // namespace Zn
