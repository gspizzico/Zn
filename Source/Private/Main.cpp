#include "Core/Log/Log.h"
#include "Core/Log/OutputDeviceManager.h"
#include "Core/Name.h"
#include "Core/HAL/BasicTypes.h"
#include "Core/Memory/Memory.h"
#include "Core/Windows/WindowsDebugOutput.h"
#include "Core/Log/LogMacros.h"
#include "Core/Memory/Allocators/StackAllocator.h"

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

		auto Allocator = StackAllocator(4096 * 10, 8);
		
		Allocator.Allocate(16);
		Allocator.Allocate(16);
		Allocator.Allocate(16);
		Allocator.SaveStatus();
		Allocator.Allocate(16);
		Allocator.Allocate(16);
		Allocator.Allocate(16);
		Allocator.SaveStatus();
		Allocator.Allocate(16);
		Allocator.Allocate(16);
		Allocator.Allocate(16);
		Allocator.RestoreStatus();
		Allocator.Allocate(16);
		Allocator.Allocate(16);
		Allocator.Allocate(16);
		Allocator.Allocate(16);
		Allocator.RestoreStatus();
		Allocator.RestoreStatus();
		Allocator.Free();

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