#include "Core/Log/Log.h"
#include "Core/Log/OutputDeviceManager.h"
#include "Core/Name.h"
#include "Core/HAL/PlatformTypes.h"
#include "Core/Memory/Memory.h"
#include "Core/Windows/WindowsMemory.h"
#include "Core/Windows/WindowsDebugOutput.h"
#include "Core/Log/LogMacros.h"

using namespace Zn;
using namespace Zn::Memory;

class Engine
{
public:
    
    void Initialize()
    {
        OutputDeviceManager::Get().RegisterOutputDevice<WindowsDebugOutput>();
        IMemory::Register(new WindowsMemory());
    }
    bool DoWork()
    {
        AutoLogCategory("TestCategory", ELogVerbosity::Verbose);
        ZN_LOG(TestCategory, ELogVerbosity::Verbose, "%s string", "ll");

        auto memStatus  = IMemory::Get()->GetMemoryStatus();
       
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