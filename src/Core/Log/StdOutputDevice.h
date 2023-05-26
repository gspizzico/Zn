#pragma once

#include <Log/OutputDevice.h>

namespace Zn
{
class StdOutputDevice : public IOutputDevice
{
  public:
    StdOutputDevice() = default;

    virtual void OutputMessage(const char* message) override;
};
} // namespace Zn
