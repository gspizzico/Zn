#include <Znpch.h>
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

DEFINE_STATIC_LOG_CATEGORY(LogMainCpp, ELogVerbosity::Verbose);

using namespace Zn;

//Screen dimension constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

int main(int argc, char* args[])
{
	CommandLine& Cmd = CommandLine::Get();

	Cmd.Initialize(args, argc);

	Engine engine{};

	engine.Start();

	engine.Shutdown();

	return 0;
}