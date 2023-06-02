#include <Core/CommandLine.h>
#include <algorithm>

using namespace Zn;

CommandLine& CommandLine::Get()
{
    static CommandLine instance;
    return instance;
}

void CommandLine::Initialize(sizet count_, char* arguments_[])
{
    arguments.reserve(count_);

    for (size_t index = 0; index < count_; ++index)
    {
        arguments.emplace_back(ToLower(arguments_[index]));
    }
}

bool CommandLine::Param(cstring param_) const
{
    return std::any_of(arguments.begin(),
                       arguments.end(),
                       [lower = ToLower(param_)](const auto& arg_)
                       {
                           return arg_.compare(lower) == 0;
                       });
}

bool CommandLine::Value(cstring param_, String& outValue_, String defaultValue_) const
{
    const String lowerParam = ToLower(param_);

    for (const String& argument : arguments)
    {
        if (argument.starts_with(lowerParam.c_str()))
        {
            cstring equalSign = argument.c_str() + strlen(param_);

            if (*equalSign == '=')
            {
                ++equalSign;
                outValue_ = equalSign;
                return true;
            }
        }
    }

    outValue_ = defaultValue_;
    return false;
}

String CommandLine::GetExeArgument() const
{
    return arguments.size() > 0 ? arguments[0] : "";
}

String CommandLine::ToLower(cstring param_) const
{
    const auto size = strlen(param_);
    String     lower;
    lower.resize(size + 1);

    for (size_t index = 0; index < size; ++index)
    {
        lower[index] = std::tolower(param_[index]);
    }
    lower[size] = '\0';

    return lower;
}
