#pragma once

#include <Core/CoreTypes.h>

namespace Zn
{
class RHIDevice
{
  public:
    static void       Create();
    static void       Destroy();
    static RHIDevice& Get();

  private:
    RHIDevice();
    ~RHIDevice();
    void CreateSwapChain();
    void CleanupSwapChain();
};
} // namespace Zn
