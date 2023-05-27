#pragma once

#include <Core/Types.h>
#include <SDL.h>

namespace Zn
{
struct InputState
{
    Vector<SDL_Event> events;
};
} // namespace Zn