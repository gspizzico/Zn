#pragma once

#include <Core/CoreTypes.h>
#include <Core/Misc/Event.h>

namespace Zn
{
struct ImGuiApp
{
    static void Create();
    static void Destroy();

    static void BeginFrame();
    static void EndFrame();
    static void Tick(float deltaTime_);

    static TMulticastEvent<float> OnTick;
};
} // namespace Zn
