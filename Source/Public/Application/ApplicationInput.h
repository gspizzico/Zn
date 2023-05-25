#pragma once

#include <Core/HAL/BasicTypes.h>
#include <SDL.h>

namespace Zn
{
struct InputState
{
    Vector<SDL_Event> events;
};
} // namespace Zn