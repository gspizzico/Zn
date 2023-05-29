#include <Windows/SDLWindow.h>
#include <sdl/SDL.h>
#include <sdl/SDL_syswm.h>

DEFINE_STATIC_LOG_CATEGORY(LogSDLWindow, ELogVerbosity::Log);

namespace
{
constexpr auto ZN_SDL_WINDOW_FLAGS = (SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_VULKAN);
}

Zn::SDLWindow::SDLWindow(int32 width_, int32 height_, cstring title_)
{
    window = SDL_CreateWindow(title_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width_, height_, ZN_SDL_WINDOW_FLAGS);

    if (!window)
    {
        ZN_LOG(LogSDLWindow, ELogVerbosity::Error, "Window could not be created! SDL_Error: %s", SDL_GetError());

        PlatformMisc::Exit(true);

        return;
    }

    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(window, &wmInfo);
    nativeHandle = (HWND) wmInfo.info.win.window;
    windowId     = SDL_GetWindowID(window);
}

Zn::SDLWindow::~SDLWindow()
{
    if (window)
    {
        SDL_DestroyWindow(window);
        window       = nullptr;
        nativeHandle = nullptr;
        windowId     = 0;
    }
}

bool Zn::SDLWindow::ProcessEvent(SDL_Event event)
{
    if (event.window.windowID == windowId)
    {
        if (event.window.event == SDL_WINDOWEVENT_CLOSE)
        {
            return false;
        }

        // TODO: Implement as events, so that we remove the strong-reference to Renderer
        if (event.window.event == SDL_WINDOWEVENT_RESIZED)
        {
            // Renderer::Get().on_window_resized();
        }
        else if (event.window.event == SDL_WINDOWEVENT_MINIMIZED)
        {
            // Renderer::Get().on_window_minimized();
        }
        else if (event.window.event == SDL_WINDOWEVENT_RESTORED)
        {
            // Renderer::Get().on_window_restored();
        }
    }

    return true;
}

Zn::WindowHandle Zn::SDLWindow::GetHandle() const
{
    return WindowHandle {
        .handle = nativeHandle,
        .id     = windowId,
    };
}
