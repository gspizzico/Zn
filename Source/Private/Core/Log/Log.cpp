#include "Core/Log/Log.h"
#include "Core/Containers/Map.h"

namespace Zn
{
    UnorderedMap<Name, uint8>& GetLogCategories()
    {
        static UnorderedMap<Name, uint8> s_LogCategories;
        return s_LogCategories;
    }

    void Log::DefineLogCategory(const Name& name, uint8 verbosity)
    {
        GetLogCategories().try_emplace(name, verbosity);
    }

    bool Log::ModifyVerbosity(const Name& name, uint8 verbosity)
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
}
