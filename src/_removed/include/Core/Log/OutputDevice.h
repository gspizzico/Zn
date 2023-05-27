#pragma once

namespace Zn
{
// Output device interface. Used to write to console/file/wherever.
class IOutputDevice abstract
{
  public:
    virtual ~IOutputDevice()
    {
    }
    virtual void OutputMessage(const char* message) = 0;
};
} // namespace Zn
