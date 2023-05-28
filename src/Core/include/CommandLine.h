#pragma once

#include <CoreTypes.h>

namespace Zn
{
class CommandLine
{
  public:
    static CommandLine& Get();

    void Initialize(char* arguments_[], sizet count_);

    bool Param(cstring param_) const;

    bool Value(cstring param_, String& outValue_, String defaultValue_ = "") const;

    String GetExeArgument() const;

    // bool Value(const char* param, String& out_value) const;

  private:
    String ToLower(cstring param_) const; // #todo implement in common library

    Vector<String> arguments;
};
} // namespace Zn
