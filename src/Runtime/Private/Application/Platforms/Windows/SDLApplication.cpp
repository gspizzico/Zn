#include <Windows/SDLApplication.h>
#include <Windows/SDLWindow.h>
#include <Application/ApplicationInput.h>
#include <sdl/SDL.h>
#include <Core/IO/IO.h>
#include <filesystem>
#include <cmath>

using namespace Zn;

DEFINE_STATIC_LOG_CATEGORY(LogSDLApplication, ELogVerbosity::Log);

namespace
{
constexpr int32 SCREEN_WIDTH  = 1280;
constexpr int32 SCREEN_HEIGHT = 720;

String GetIORootPath()
{
    std::filesystem::path executablePath(CommandLine::Get().GetExeArgument());

    // TODO: Fix path after refactor directory change
    return executablePath.parent_path().parent_path().parent_path().string();
}
} // namespace

namespace Zn
{

void SDLApplication::Initialize()
{
    ZN_TRACE_QUICKSCOPE();

    IO::Initialize(GetIORootPath());

    check(!isInitialized);

    if (SDL_InitSubSystem(SDL_INIT_VIDEO /*TODO: add more flags here*/) < 0)
    {
        ZN_LOG(LogSDLApplication, ELogVerbosity::Error, "SDL could not initialize. SDL_Error: %s", SDL_GetError());

        PlatformMisc::Exit(true);
    }

    title = "Zn-Engine";

    window = std::make_shared<SDLWindow>(SCREEN_WIDTH, SCREEN_HEIGHT, title);

    inputState = std::make_shared<InputState>();

    isInitialized = true;

    ZN_LOG(LogSDLApplication, ELogVerbosity::Log, "Application initialized.");
}

void SDLApplication::Shutdown()
{
    check(isInitialized);

    window = nullptr;

    inputState = nullptr;

    SDL_Quit();
}

bool SDLApplication::ProcessOSEvents(float deltaTime_)
{
    check(isInitialized);

    deltaTime_ = std::max(deltaTime_, FLT_EPSILON);

    SDL_Event event;

    String titleString = title + String(" FPS: ") + std::to_string(static_cast<int32>(floor(1.f / deltaTime_)));
    window->SetTitle(titleString.c_str());

    inputState->events.clear();

    while (SDL_PollEvent(&event) != 0)
    {
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

        externalEventProcessor.Broadcast(&event);
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
