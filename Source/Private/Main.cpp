#include <Znpch.h>
#include <Application/Application.h>
#include "Core/Log/OutputDeviceManager.h"
#include "Core/Name.h"
#include "Core/Memory/Memory.h"
#include "Windows/WindowsDebugOutput.h"
#include "Core/CommandLine.h"
#include "Core/Memory/Allocators/StackAllocator.h"
#include "Core/Memory/Allocators/PageAllocator.h"
#include "Core/Memory/Allocators/TLSFAllocator.h"
#include "Automation/AutomationTestManager.h"
#include "Core/HAL/Misc.h"
#include "Engine/Engine.h"
#include <algorithm>
#include <utility>
#include <random>
#include <numeric>
#include <SDL.h>
#include <Core/Time/Time.h>
#include <Core/IO/IO.h>

DEFINE_STATIC_LOG_CATEGORY(LogMainCpp, ELogVerbosity::Verbose);

using namespace Zn;

int main(int argc, char* args[])
{
    // Initialize command line arguments
    CommandLine::Get().Initialize(args, argc);

    // Initialize Application layer.
    Application& app = Application::Get();
    app.Initialize();

#if ZN_DEBUG
    if (CommandLine::Get().Param("-CompileShaders"))
    {
        ZN_LOG(LogMainCpp, ELogVerbosity::Log, "Running compile shaders.");

        String command = "py " + IO::GetAbsolutePath("scripts/vk_compile_shaders.py");

        if (i32 result = std::system(command.c_str()); result != 0)
        {
            ZN_LOG(LogMainCpp, ELogVerbosity::Error, "Failed to compile shaders.");
            return -1;
        }
    }
#endif

    // Initialize Engine layer.
    Engine* engine = new Engine();
    engine->Initialize();

    f64 lastFrameTime    = Time::Seconds();
    f64 currentFrameTime = lastFrameTime;

    while (!app.WantsToExit())
    {
        currentFrameTime = Time::Seconds();

        f32 deltaTime = static_cast<f32>(currentFrameTime - lastFrameTime);

        lastFrameTime = currentFrameTime;

        if (!app.ProcessOSEvents(deltaTime))
        {
            break;
        }

        engine->Update(deltaTime);
    }

    engine->Shutdown();

    delete engine;

    Application::Get().Shutdown();

    return 0;
}
