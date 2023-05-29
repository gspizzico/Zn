#include <Windows/SDLApplication.h>
#include <Windows/SDLWindow.h>
#include <ApplicationInput.h>
#include <sdl/SDL.h>

DEFINE_STATIC_LOG_CATEGORY(LogSDLApplication, ELogVerbosity::Log);

namespace
{
constexpr int32 SCREEN_WIDTH  = 1280;
constexpr int32 SCREEN_HEIGHT = 720;

bool GPlatformApplicationInitialized = false;
} // namespace

namespace Zn
{

void Platform_InitializeApplication()
{
    check(GPlatformApplicationInitialized == false);

    SDLApplication* application = new SDLApplication();

    application->Initialize();

    GPlatformApplicationInitialized = true;
}

void Platform_ShutdownApplication()
{
    check(GPlatformApplicationInitialized);

    SDLApplication* application = static_cast<SDLApplication*>(&Application::Get());

    application->Shutdown();
}

void SDLApplication::Initialize()
{
    ZN_TRACE_QUICKSCOPE();

    check(!isInitialized);

    if (SDL_InitSubSystem(SDL_INIT_VIDEO /*TODO: add more flags here*/) < 0)
    {
        ZN_LOG(LogSDLApplication, ELogVerbosity::Error, "SDL could not initialize. SDL_Error: %s", SDL_GetError());

        PlatformMisc::Exit(true);
    }

    window = std::make_shared<SDLWindow>(SCREEN_WIDTH, SCREEN_HEIGHT, "Zn-Engine");

    inputState = std::make_shared<InputState>();

    isInitialized = true;

    SetApplication(this);

    ZN_LOG(LogSDLApplication, ELogVerbosity::Log, "Application initialized.");
}

void SDLApplication::Shutdown()
{
    check(isInitialized);

    window = nullptr;

    inputState = nullptr;

    SDL_Quit();

    SetApplication(nullptr);
}
bool SDLApplication::ProcessOSEvents(float deltaTime)
{
    check(isInitialized);

    SDL_Event event;

    inputState->events.clear();

    while (SDL_PollEvent(&event) != 0)
    {
        // imgui_process_event(event);

        if (event.type == SDL_QUIT)
        {
            isExitRequested = true;
        }
        else if (event.type == SDL_WINDOWEVENT)
        {
            isExitRequested |= !window->ProcessEvent(event);
        }
        else if (event.type >= SDL_KEYDOWN && event.type <= SDL_CONTROLLERSENSORUPDATE)
        {
            inputState->events.push_back(event);
        }
    }

    return !isExitRequested;
}
WindowHandle SDLApplication::GetWindowHandle() const
{
    check(isInitialized);

    return window->GetHandle();
}
SharedPtr<InputState> SDLApplication::GetInputState() const
{
    return inputState;
}
void SDLApplication::RequestExit(cstring exitReason_)
{
    if (!isExitRequested)
    {
        ZN_LOG(LogSDLApplication, ELogVerbosity::Log, "Requested application exit. Reason: %s", exitReason_);

        isExitRequested = true;
    }
}
bool SDLApplication::WantsToExit() const
{
    return false;
}
} // namespace Zn
