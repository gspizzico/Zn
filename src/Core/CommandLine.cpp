#include <Corepch.h>
#include <CommandLine.h>
#include <algorithm>

using namespace Zn;

CommandLine& CommandLine::Get()
{
    static CommandLine Instance;
    return Instance;
}

void CommandLine::Initialize(char* arguments[], size_t count)
{
    arguments_.reserve(count);

    for (size_t index = 0; index < count; ++index)
    {
        arguments_.emplace_back(ToLower(arguments[index]));
    }
}

bool CommandLine::Param(const char* param) const
{
    return std::any_of(arguments_.begin(),
                       arguments_.end(),
                       [lower = ToLower(param)](const auto& arg)
                       {
                           return arg.compare(lower) == 0;
                       });
}

bool CommandLine::Value(cstring param, String& value, String defaultValue) const
{
    const String lowerParam = ToLower(param);

    for (const String& argument : arguments_)
    {
        if (argument.starts_with(lowerParam.c_str()))
        {
            cstring equalSign = argument.c_str() + strlen(param);

            if (*equalSign == '=')
            {
                ++equalSign;
                value = equalSign;
                return true;
            }
        }
    }

    value = defaultValue;
    return false;
}

String CommandLine::GetExeArgument() const
{
    return arguments_.size() > 0 ? arguments_[0] : "";
}

String CommandLine::ToLower(const char* param) const
{
    const auto size = strlen(param);
    String     lower;
    lower.resize(size + 1);

    for (size_t index = 0; index < strlen(param); ++index)
    {
        lower[index] = std::tolower(param[index]);
    }
    lower[size] = '\0';

    return lower;
}
