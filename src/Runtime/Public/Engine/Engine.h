#pragma once

#include <Core/CoreTypes.h>

namespace Zn
{
class Engine
{
  public:
    static void Create();
    static void Destroy();
    static void Tick(float deltaTime_);
};
} // namespace Zn