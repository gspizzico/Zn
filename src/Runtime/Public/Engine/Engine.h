#pragma once

#include <CoreTypes.h>

namespace Zn
{
class Engine
{
  public:
    static int32 Launch();

  private:
    static void Tick(float deltaTime_);
};
} // namespace Zn
