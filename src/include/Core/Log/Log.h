#pragma once

#include <Core/Types.h>
#include <Core/Name.h>
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
    Name name;

    ELogVerbosity verbosity;

    bool IsSuppressed(ELogVerbosity verbosity_) const
    {
        return verbosity > verbosity_;
    }
};

typedef void (*PFN_LogMessageCallback)(cstring message_);

// Utility class for logging functionalities.
class Log
{
  public:
    // It provides only static methods, does not need a constructor.
    Log() = delete;

    static void AddLogCategory(SharedPtr<LogCategory> category_);

    static bool ModifyVerbosity(const Name& name_, ELogVerbosity verbosity_);

    // Variadic function used to log a message.
    template<typename... Args>
    static void LogMsg(const Name& category_, ELogVerbosity verbosity_, const char* format_, Args&&... args_);

    static void SetLogMessageCallback(PFN_LogMessageCallback callback_);

  private:
    // Log Category getter
    static SharedPtr<LogCategory> GetLogCategory(const Name& name_);

    static void LogMsgInternal(const Name& category_, ELogVerbosity verbosity_, const char* message_);

    static const char* ToCString(ELogVerbosity verbosity_);
};

// Utility struct that autoregisters a log category.
struct AutoLogCategory
{
    AutoLogCategory() = default;

    AutoLogCategory(Name name_, ELogVerbosity verbosity_)
    {
        logCategory = std::make_shared<LogCategory>(LogCategory {name_, verbosity_});

        Zn::Log::AddLogCategory(logCategory);
    }

    SharedPtr<LogCategory> logCategory;

    LogCategory& Category()
    {
        return *logCategory;
    }
};

template<typename... Args>
inline void Log::LogMsg(const Name& category_, ELogVerbosity verbosity_, const char* format_, Args&&... args_)
{
    const auto bufferSize = std::snprintf(nullptr, 0, format_, std::forward<Args>(args_)...);

    // #todo [Memory] - use custom allocator
    Vector<char> buffer((size_t) bufferSize + 1ull); // note +1 for null terminator

    std::snprintf(&buffer[0], buffer.size(), format_, std::forward<Args>(args_)...);

    LogMsgInternal(category_, verbosity_, &buffer[0]);
}
} // namespace Zn
