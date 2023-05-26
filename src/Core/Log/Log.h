#pragma once

#include <Types.h>
#include <Log/OutputDeviceManager.h>
#include <Name.h>
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
    Name m_Name;

    ELogVerbosity m_Verbosity;

    bool IsSuppressed(ELogVerbosity verbosity) const
    {
        return m_Verbosity > verbosity;
    }
};

// Utility class for logging functionalities.
class Log
{
  public:
    // It provides only static methods, does not need a constructor.
    Log() = delete;

    static void AddLogCategory(SharedPtr<LogCategory> category);

    static bool ModifyVerbosity(const Name& name, ELogVerbosity verbosity);

    // Variadic function used to log a message.
    template<typename... Args>
    static void LogMsg(const Name& category, ELogVerbosity verbosity, const char* format, Args&&... args);

  private:
    // Log Category getter
    static SharedPtr<LogCategory> GetLogCategory(const Name& name);

    static void LogMsgInternal(const Name& category, ELogVerbosity verbosity, const char* message);

    static const char* ToCString(ELogVerbosity verbosity);
};

// Utility struct that autoregisters a log category.
struct AutoLogCategory
{
    AutoLogCategory() = default;

    AutoLogCategory(Name name, ELogVerbosity verbosity)
    {
        m_LogCategory = std::make_shared<LogCategory>(LogCategory {name, verbosity});

        Zn::Log::AddLogCategory(m_LogCategory);
    }

    SharedPtr<LogCategory> m_LogCategory;

    LogCategory& Category()
    {
        return *m_LogCategory;
    }
};

template<typename... Args>
inline void Log::LogMsg(const Name& category, ELogVerbosity verbosity, const char* format, Args&&... args)
{
    const auto MessageBufferSize = std::snprintf(nullptr, 0, format, std::forward<Args>(args)...);

    // #todo [Memory] - use custom allocator
    Vector<char> MessageBuffer((size_t) MessageBufferSize + 1ull); // note +1 for null terminator

    std::snprintf(&MessageBuffer[0], MessageBuffer.size(), format, std::forward<Args>(args)...);

    LogMsgInternal(category, verbosity, &MessageBuffer[0]);
}
} // namespace Zn

#define CONCAT(A, B)            A##B

#define CATEGORY_VAR_NAME(Name) CONCAT(Name, Category)

#define GET_CATEGORY(Name)      Zn::LogCategories::CATEGORY_VAR_NAME(Name)

// Log declarations/definitions macros.
#define DECLARE_LOG_CATEGORY(Name)                                                                                                         \
    namespace Zn::LogCategories                                                                                                            \
    {                                                                                                                                      \
    extern Zn::AutoLogCategory CATEGORY_VAR_NAME(Name);                                                                                    \
    }

#define DEFINE_LOG_CATEGORY(Name, Verbosity) Zn::AutoLogCategory Zn::LogCategories::CATEGORY_VAR_NAME(Name) {#Name, Verbosity};

#define DEFINE_STATIC_LOG_CATEGORY(Name, Verbosity)                                                                                        \
    namespace Zn::LogCategories                                                                                                            \
    {                                                                                                                                      \
    static Zn::AutoLogCategory CATEGORY_VAR_NAME(Name) {#Name, Verbosity};                                                                 \
    }
