#pragma once

#include <Types.h>
#include <Containers/Vector.h>

namespace Zn
{
class CommandLine
{
  public:
    static CommandLine& Get();

    void Initialize(char* arguments[], size_t count);

    bool Param(const char* param) const;

    bool Value(cstring param, String& value, String defaultValue = "") const;

    String GetExeArgument() const;

    // bool Value(const char* param, String& out_value) const;

  private:
    String ToLower(const char* param) const; // #todo implement in common library

    Vector<String> arguments_;
};
} // namespace Zn
