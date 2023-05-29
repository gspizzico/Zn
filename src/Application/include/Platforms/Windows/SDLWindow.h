#pragma once

#include <CoreMinimal.h>
#include <Application.h>
#include <sdl/SDL.h>

namespace Zn
{
class SDLWindow
{
  public:
    SDLWindow(int32 width_, int32 height_, cstring title_);

    ~SDLWindow();

    bool ProcessEvent(SDL_Event event);

    WindowHandle GetHandle() const;

  private:
    SDL_Window* window {nullptr};
    uint32      windowId {0};
    void*       nativeHandle {nullptr};
};
} // namespace Zn
