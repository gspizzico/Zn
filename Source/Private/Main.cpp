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

DEFINE_STATIC_LOG_CATEGORY(LogMainCpp, ELogVerbosity::Verbose);

using namespace Zn;

int main(int argc, char* args[])
{
	// Initialize command line arguments
	CommandLine::Get().Initialize(args, argc);

	// Initialize Application layer.
	Application& app = Application::Get();
	app.Initialize();

	// Initialize Engine layer.
	Engine* engine = new Engine();
	engine->Initialize();

	f64 lastFrameTime = Time::Seconds();
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