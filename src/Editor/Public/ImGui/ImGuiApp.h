#pragma once

#include <Core/CoreTypes.h>

namespace Zn
{
struct ImGuiApp
{
    static void Create();
    static void Destroy();

    static void BeginFrame();
    static void Tick(float deltaTime_);
};
} // namespace Zn
