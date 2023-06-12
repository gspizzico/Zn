#include <Core/Log/Log.h>
#include <Core/Time/Time.h>
#include <Core/CoreAssert.h>
#include <Core/CorePlatform.h>

using namespace Zn;

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

static constexpr uint16 GMaxCategories = 1024;
static LogCategory      GCategories[GMaxCategories];
static uint16           GNumCategories = 0;
} // namespace

namespace Zn
{
LogCategoryHandle Log::AddLogCategory(const LogCategory& category_)
{
    check(GNumCategories < GMaxCategories);

    uint16 index = GNumCategories;

    GCategories[index] = category_;

    ++GNumCategories;

    return LogCategoryHandle {
        .handle = index,
    };
}

bool Log::ModifyVerbosity(cstring name_, ELogVerbosity verbosity_)
{
    if (LogCategory* category = GetLogCategory(name_))
    {
        category->verbosity = verbosity_;
        return true;
    }

    return false;
}

void Log::SetLogMessageCallback(PFN_LogMessageCallback callback_)
{
    GLogCallback = callback_;
}

const LogCategory& Log::GetLogCategory(LogCategoryHandle handle_)
{
    check(handle_.handle < GNumCategories);
    return GCategories[handle_.handle];
}

LogCategory* Log::GetLogCategory(cstring name_)
{
    for (uint16 index = 0; index < GNumCategories; ++index)
    {
        if (_strcmpi(GCategories[index].name, name_))
        {
            return &GCategories[index];
        }
    }

    return nullptr;
}

void Log::LogMsgInternal(LogCategoryHandle handle_, ELogVerbosity verbosity_, const char* message_)
{
    check(handle_.handle < GNumCategories);

    LogCategory& category = GCategories[handle_.handle];

    char buffer[1024];

    // [TimeStamp] [LogCategory] [LogVerbosity]: Message \n
    static constexpr cstring format = "[%s] [%s] %s: %s\n";

    std::snprintf(buffer, sizeof(buffer), format, Time::Now().c_str(), category.name, ToCString(verbosity_), message_);

    if (GLogCallback)
    {
        GLogCallback(buffer);
    }
}

cstring Log::ToCString(ELogVerbosity verbosity_)
{
    check(verbosity_ != ELogVerbosity::MAX);

    static cstring kVerbosities[] = {"Verbose", "Log", "Warning", "Error"};

    return kVerbosities[(int32) verbosity_];
}
} // namespace Zn
