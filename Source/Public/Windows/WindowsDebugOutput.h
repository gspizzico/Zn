#pragma once

#include "Core/Log/OutputDevice.h"

namespace Zn
{
class WindowsDebugOutput : public IOutputDevice
{
  public:
    WindowsDebugOutput() = default;

    virtual void OutputMessage(const char* message);
};
} // namespace Zn