#include <Znpch.h>
#include <Application/Application.h>
#include <Core/IO/IO.h>
#include <Core/CommandLine.h>
#include <Core/Log/OutputDeviceManager.h>
#include <Core/Log/StdOutputDevice.h>
#include <Core/HAL/SDL/SDLWrapper.h>
#include <Windows/WindowsDebugOutput.h> // #TODO Move to somewhere not platform specific
#include <Application/Window.h>
#include <SDL.h>
#include <ImGui/ImGuiWrapper.h>
#include <Application/ApplicationInput.h>

using namespace Zn;

DEFINE_STATIC_LOG_CATEGORY(LogApplication, ELogVerbosity::Log);

namespace
{
constexpr i32 SCREEN_WIDTH  = 1280;
constexpr i32 SCREEN_HEIGHT = 720;
} // namespace

Application& Zn::Application::Get()
{
    static Application instance {};
    return instance;
}

void Application::Initialize()
{
    if (!is_initialized)
    {
        ZN_TRACE_QUICKSCOPE();

        IO::Initialize();

        OutputDeviceManager::Get().RegisterOutputDevice<WindowsDebugOutput>();

        if (CommandLine::Get().Param("-std"))
        {
            OutputDeviceManager::Get().RegisterOutputDevice<StdOutputDevice>();
        }

        SDLWrapper::Initialize();

        // Create Window

        window = std::make_shared<Window>(SCREEN_WIDTH, SCREEN_HEIGHT, "Zn-Engine");

        input = std::make_shared<InputState>();

        is_initialized = true;

        ZN_LOG(LogApplication, ELogVerbosity::Log, "Application Initialized");
    }
    else
    {
        ZN_LOG(LogApplication, ELogVerbosity::Error, "Application was already initialized!");
    }
}

void Application::Shutdown()
{
    if (is_initialized)
    {
        SDLWrapper::Shutdown();
    }
}

bool Application::ProcessOSEvents(float deltaTime)
{
    check(is_initialized);

    SDL_Event event;

    input->events.clear();

    while (SDL_PollEvent(&event) != 0)
    {
        imgui_process_event(event);

        if (event.type == SDL_QUIT)
        {
            is_exit_requested = true;
        }
        else if (event.type == SDL_WINDOWEVENT)
        {
            is_exit_requested |= !window->ProcessEvent(event);
        }
        else if (event.type >= SDL_KEYDOWN && event.type <= SDL_CONTROLLERSENSORUPDATE)
        {
            input->events.push_back(event);
        }
    }

    return !is_exit_requested;
}

SharedPtr<Window> Application::GetWindow() const
{
    return window;
}

SharedPtr<InputState> Zn::Application::GetInputState() const
{
    return input;
}

void Application::RequestExit(String exitReason)
{
    if (!is_exit_requested)
    {
        ZN_LOG(LogApplication, ELogVerbosity::Log, "Requested application exit. Reason: %s", exitReason.c_str());

        is_exit_requested = true;
    }
}

bool Application::WantsToExit() const
{
    return is_exit_requested;
}
