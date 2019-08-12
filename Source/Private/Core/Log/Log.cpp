#include "Core/Log/Log.h"
#include "Core/Containers/Map.h"
#include "Core/Time/Time.h"

namespace Zn
{
    UnorderedMap<Name, ELogVerbosity>& GetLogCategories()
    {
        static UnorderedMap<Name, ELogVerbosity> s_LogCategories;
        return s_LogCategories;
    }

    void Log::DefineLogCategory(const Name& name, ELogVerbosity verbosity)
    {
        GetLogCategories().try_emplace(name, verbosity);
    }

    bool Log::ModifyVerbosity(const Name& name, ELogVerbosity verbosity)
    {
        if (auto It = GetLogCategories().find(name); It != GetLogCategories().end())
        {
            It->second = verbosity;
            return true;
        }

        return false;
    }

    std::optional<LogCategory> Log::GetLogCategory(const Name& name)
    {
        std::optional<LogCategory> Result;

        if (auto It = GetLogCategories().find(name); It != GetLogCategories().end())
        {
            Result = LogCategory{ It->first, It->second };
        }

        return Result;
    }

    void Log::LogMsgInternal(const Name & category, ELogVerbosity verbosity, const char* message)
    {   
        constexpr auto LogFormat = "[%s]\t[%s]\t%s:\t%s\n";

        auto Now = SystemClock::now();
        auto TimeString = Time::ToString(Now);
        const auto TimeCString = TimeString.c_str();

        auto LogCategoryCString = category.CString();
        auto LogVerbosityCString = ToCString(verbosity);

        const auto LogFormatSize = std::snprintf(nullptr, 0, LogFormat, TimeCString, LogCategoryCString, LogVerbosityCString, message);

        Vector<char> LogMessageBuffer(LogFormatSize + 1); // note +1 for null terminator

        std::snprintf(&LogMessageBuffer[0], LogMessageBuffer.size(), LogFormat, TimeCString, LogCategoryCString, LogVerbosityCString, message);

        OutputDeviceManager::Get().OutputMessage(&LogMessageBuffer[0]);
    }

    const char* Log::ToCString(ELogVerbosity verbosity)
    {
        static const Name sVerbose    { "Verbose" };
        static const Name sLog        { "Log" };
        static const Name sWarning    { "Warning" };
        static const Name sError      { "Error" };

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
            _ASSERT(false);
            return nullptr;
        }
    }
}
