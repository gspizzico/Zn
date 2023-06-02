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

    void SetTitle(cstring title_);

    bool ProcessEvent(SDL_Event event);

    WindowHandle GetHandle() const;

  private:
    SDL_Window* window {nullptr};
    uint32      windowId {0};
    void*       nativeHandle {nullptr};
    int32       width {0};
    int32       height {0};
};
} // namespace Zn
