#pragma once

#include <CoreMinimal.h>
#include <sdl/SDL.h> // TODO: TEMP

namespace Zn
{
struct InputState
{
    Vector<SDL_Event> events;
};
} // namespace Zn
