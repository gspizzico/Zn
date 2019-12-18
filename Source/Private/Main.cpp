#include "Core/Log/Log.h"
#include "Core/Log/OutputDeviceManager.h"
#include "Core/Name.h"
#include "Core/HAL/BasicTypes.h"
#include "Core/Memory/Memory.h"
#include "Core/Windows/WindowsDebugOutput.h"
#include "Core/Log/LogMacros.h"
#include "Core/Memory/Allocators/StackAllocator.h"
#include "Core/Memory/Allocators/PageAllocator.h"
#include "Core/Memory/Allocators/TLSFAllocator.h"
#include "Automation/AutomationTestManager.h"
#include "Core/HAL/Misc.h"
#include <algorithm>
#include <utility>
#include <random>
#include <numeric>

DEFINE_STATIC_LOG_CATEGORY(LogMainCpp, ELogVerbosity::Verbose);

using namespace Zn;

class Engine
{
public:

	void Initialize()
	{
		OutputDeviceManager::Get().RegisterOutputDevice<WindowsDebugOutput>();
	}

	bool DoWork()
	{
		Automation::AutomationTestManager::Get().ExecuteStartupTests();
		return false;
	}
	void Shutdown()
	{
	}
};

int main()
{
	Engine engine;
	engine.Initialize();
	while (engine.DoWork());
	engine.Shutdown();
	return 0;
}