#include "Core/Log/Log.h"
#include "Core/Log/OutputDeviceManager.h"
#include "Core/Name.h"
#include "Core/HAL/BasicTypes.h"
#include "Core/Memory/Memory.h"
#include "Core/Windows/WindowsDebugOutput.h"
#include "Core/Log/LogMacros.h"
#include "Core/Memory/Allocators/LinearAllocator.h"

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
       LinearAllocator Allocator(1024*2, 256);
       for(int index = 0; index < 4; ++index)
        Allocator.Allocate(512);
       
       Allocator.Free();

       /* AutoLogCategory("TestCategory", ELogVerbosity::Verbose);
        ZN_LOG(TestCategory, ELogVerbosity::Verbose, "%s string", "ll");

        auto memStatus  = Memory::GetMemoryStatus();*/
       
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