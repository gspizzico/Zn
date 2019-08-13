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
       LinearAllocator Allocator(64, 1);
       constexpr auto namesize = sizeof(Name);
       auto Ptr1 = Allocator.Allocate(namesize);
       Name* MyName = new (Ptr1) Name("MyName");
       
       auto Ptr2 = Allocator.Allocate(16);
       auto Ptr3 = Allocator.Allocate(4);
       auto Ptr4 = Allocator.Allocate(1);
       MyName->~Name();
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