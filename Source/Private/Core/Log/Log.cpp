#include "Core/Log/Log.h"
#include "Core/Containers/Map.h"

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
    void Log::LogMsgInternal(const Name & category, const char* message)
    {
        //#todo [Time] - Add TimeStamp
        constexpr auto LogFormat = "[%s] : %s \n";

        auto LogCategoryCString = category.CString();

        const auto LogFormatSize = std::snprintf(nullptr, 0, LogFormat, LogCategoryCString, message);

        Vector<char> LogMessageBuffer(LogFormatSize + 1); // note +1 for null terminator

        std::snprintf(&LogMessageBuffer[0], LogMessageBuffer.size(), LogFormat, LogCategoryCString, message);

        OutputDeviceManager::Get().OutputMessage(&LogMessageBuffer[0]);
    }
}
