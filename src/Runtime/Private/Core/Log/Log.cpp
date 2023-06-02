#include <Log/Log.h>
#include <Time/Time.h>
#include <CoreAssert.h>
#include <CorePlatform.h>

namespace
{
Zn::PFN_LogMessageCallback GLogCallback = nullptr;

void DefaultLogMsg(cstring message_)
{
    Zn::PlatformMisc::LogDebug(message_);
    Zn::PlatformMisc::LogConsole(message_);
}

struct AutoMessageCallback
{
    AutoMessageCallback()
    {
        Zn::Log::SetLogMessageCallback(DefaultLogMsg);
    }
};

AutoMessageCallback GDefaultMessageCallback {};
} // namespace

namespace Zn
{
// Internal use only. Map of registered log categories.
UnorderedMap<Name, SharedPtr<LogCategory>>& GetLogCategories()
{
    static UnorderedMap<Name, SharedPtr<LogCategory>> logCategories;
    return logCategories;
}

void Log::AddLogCategory(SharedPtr<LogCategory> category_)
{
    GetLogCategories().try_emplace(category_->name, category_);
}

bool Log::ModifyVerbosity(const Name& name_, ELogVerbosity verbosity_)
{
    if (auto it = GetLogCategories().find(name_); it != GetLogCategories().end())
    {
        it->second->verbosity = verbosity_;
        return true;
    }

    return false;
}

void Log::SetLogMessageCallback(PFN_LogMessageCallback callback_)
{
    GLogCallback = callback_;
}

SharedPtr<LogCategory> Log::GetLogCategory(const Name& name_)
{
    if (auto it = GetLogCategories().find(name_); it != GetLogCategories().end())
    {
        return it->second;
    }

    return nullptr;
}

void Log::LogMsgInternal(const Name& category_, ELogVerbosity verbosity_, const char* message_)
{
    // Printf wrapper, since it's called two times, the first one to get the size of the buffer, the second one to write to it.
    auto execute_printf = [timeString = Time::Now(), pLogCategory = category_.CString(), pLogVerbosity = ToCString(verbosity_), message_](
                              char* buffer_, size_t size_) -> size_t
    {
        // Log Format -> [TimeStamp]    [LogCategory]   [LogVerbosity]: Message \n
        constexpr auto format = "[%s]\t[%s]\t%s:\t%s\n";

        return std::snprintf(buffer_, size_, format, timeString.c_str(), pLogCategory, pLogVerbosity, message_);
    };

    const auto bufferSize = execute_printf(nullptr, 0);

    Vector<char> buffer(bufferSize + 1); // note +1 for null terminator

    execute_printf(&buffer[0], buffer.size());

    if (GLogCallback)
    {
        GLogCallback(&buffer[0]);
    }
}

const char* Log::ToCString(ELogVerbosity verbosity_)
{
    static const Name kVerbose {"Verbose"};
    static const Name kLog {"Log"};
    static const Name kWarning {"Warning"};
    static const Name kError {"Error"};

    switch (verbosity_)
    {
    case ELogVerbosity::Verbose:
        return kVerbose.CString();
        break;
    case ELogVerbosity::Log:
        return kLog.CString();
        break;
    case ELogVerbosity::Warning:
        return kWarning.CString();
        break;
    case ELogVerbosity::Error:
        return kError.CString();
        break;
    default:
        check(false);
        return nullptr;
    }
}
} // namespace Zn
