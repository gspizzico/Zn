#pragma once

#include <Core/CoreTypes.h>

namespace Zn
{
class RHIDevice
{
  public:
    RHIDevice();

    ~RHIDevice();

  private:
    void CreateSwapChain();
    void CleanupSwapChain();
};
} // namespace Zn
