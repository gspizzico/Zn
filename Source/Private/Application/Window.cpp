#include <Znpch.h>
#include <Application/Window.h>
#include <Core/HAL/SDL/SDLWrapper.h>
#include <SDL.h>
#include <SDL_syswm.h>
#include <ImGui/ImGuiWrapper.h>
#include <Rendering/Renderer.h>

DEFINE_STATIC_LOG_CATEGORY(LogWindow, ELogVerbosity::Log);

using namespace Zn;

Window::Window(const int width, const int height, const String& title)
{
    check(SDLWrapper::IsInitialized());

    // Create window at default position

    static auto ZN_SDL_WINDOW_FLAGS = (SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_VULKAN);

    window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, ZN_SDL_WINDOW_FLAGS);

    if (window == NULL)
    {
        ZN_LOG(LogWindow, ELogVerbosity::Error, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return; // #todo - exception / crash / invalid
    }

    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(window, &wmInfo);
    nativeHandle = (HWND) wmInfo.info.win.window;
    windowID     = SDL_GetWindowID(window);
}

Window::~Window()
{
    if (window != nullptr)
    {
        SDL_DestroyWindow(window);

        window = nullptr;
    }
}

bool Zn::Window::ProcessEvent(SDL_Event event)
{
    if (event.window.windowID == windowID)
    {
        if (event.window.event == SDL_WINDOWEVENT_CLOSE)
        {
            return false;
        }

        // TODO: Implement as events, so that we remove the strong-reference to Renderer
        if (event.window.event == SDL_WINDOWEVENT_RESIZED)
        {
            Renderer::Get().on_window_resized();
        }
        else if (event.window.event == SDL_WINDOWEVENT_MINIMIZED)
        {
            Renderer::Get().on_window_minimized();
        }
        else if (event.window.event == SDL_WINDOWEVENT_RESTORED)
        {
            Renderer::Get().on_window_restored();
        }
    }

    return true;
}
