#include <Corepch.h>
#include "Log/Log.h"
#include "Containers/Map.h"
#include "Time/Time.h"
#include "Platform.h"

namespace
{
Zn::PFN_LogMessageCallback GLogCallback = nullptr;

void DefaultLogMsg(cstring message)
{
    Zn::PlatformMisc::LogDebug(message);
    Zn::PlatformMisc::LogConsole(message);
}

struct AutoMessageCallback
{
    AutoMessageCallback()
    {
        Zn::Log::SetLogMessageCallback(DefaultLogMsg);
    }
};
} // namespace
namespace Zn
{
// Internal use only. Map of registered log categories.
UnorderedMap<Name, SharedPtr<LogCategory>>& GetLogCategories()
{
    static UnorderedMap<Name, SharedPtr<LogCategory>> s_LogCategories;
    return s_LogCategories;
}

void Log::AddLogCategory(SharedPtr<LogCategory> category)
{
    GetLogCategories().try_emplace(category->m_Name, category);
}

bool Log::ModifyVerbosity(const Name& name, ELogVerbosity verbosity)
{
    if (auto It = GetLogCategories().find(name); It != GetLogCategories().end())
    {
        It->second->m_Verbosity = verbosity;
        return true;
    }

    return false;
}

void Log::SetLogMessageCallback(PFN_LogMessageCallback callback)
{
    GLogCallback = callback;
}

SharedPtr<LogCategory> Log::GetLogCategory(const Name& name)
{
    if (auto It = GetLogCategories().find(name); It != GetLogCategories().end())
    {
        return It->second;
    }

    return nullptr;
}

void Log::LogMsgInternal(const Name& category, ELogVerbosity verbosity, const char* message)
{
    // Printf wrapper, since it's called two times, the first one to get the size of the buffer, the second one to write to it.
    auto ExecPrintf = [TimeString = Time::Now(), pLogCategory = category.CString(), pLogVerbosity = ToCString(verbosity), message](
                          char* buffer, size_t size) -> size_t
    {
        // Log Format -> [TimeStamp]    [LogCategory]   [LogVerbosity]: Message \n
        constexpr auto LogFormat = "[%s]\t[%s]\t%s:\t%s\n";

        return std::snprintf(buffer, size, LogFormat, TimeString.c_str(), pLogCategory, pLogVerbosity, message);
    };

    const auto BufferSize = ExecPrintf(nullptr, 0);

    Vector<char> Buffer(BufferSize + 1); // note +1 for null terminator

    ExecPrintf(&Buffer[0], Buffer.size());

    if (GLogCallback)
    {
        GLogCallback(&Buffer[0]);
    }
}

const char* Log::ToCString(ELogVerbosity verbosity)
{
    static const Name sVerbose {"Verbose"};
    static const Name sLog {"Log"};
    static const Name sWarning {"Warning"};
    static const Name sError {"Error"};

    switch (verbosity)
    {
    case ELogVerbosity::Verbose:
        return sVerbose.CString();
        break;
    case ELogVerbosity::Log:
        return sLog.CString();
        break;
    case ELogVerbosity::Warning:
        return sWarning.CString();
        break;
    case ELogVerbosity::Error:
        return sError.CString();
        break;
    default:
        check(false);
        return nullptr;
    }
}
} // namespace Zn
