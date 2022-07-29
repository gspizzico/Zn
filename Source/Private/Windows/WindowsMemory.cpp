#include "Windows/WindowsMemory.h"
#include "Windows/WindowsMisc.h"
#include "Windows/WindowsCommon.h"
#include "Core/Build.h"

#define ZN_WINDOWS_TRACK_MEMORY (ZN_TRACK_MEMORY && !ZN_RELEASE) && 0

#if ZN_WINDOWS_TRACK_MEMORY 
#include <VSCustomNativeHeapEtwProvider.h>
#endif

namespace Zn
{
    MemoryStatus WindowsMemory::GetMemoryStatus()
    {
        _MEMORYSTATUSEX WinMemStatus;
        WinMemStatus.dwLength = sizeof(WinMemStatus);

        GlobalMemoryStatusEx(&WinMemStatus);

        return
        {
            (uint64)WinMemStatus.dwMemoryLoad,
            (uint64)WinMemStatus.ullTotalPhys,              (uint64)WinMemStatus.ullAvailPhys,
            (uint64)WinMemStatus.ullTotalPageFile,          (uint64)WinMemStatus.ullAvailPageFile,
            (uint64)WinMemStatus.ullTotalVirtual,           (uint64)WinMemStatus.ullAvailVirtual,
            (uint64)WinMemStatus.ullAvailExtendedVirtual
        };
    }
#if ZN_WINDOWS_TRACK_MEMORY 
	auto HeapTracker = std::make_unique<VSHeapTracker::CHeapTracker>("Zn::WindowsMemory");
#endif

	void WindowsMemory::TrackAllocation(void* address, size_t size)
	{
#if ZN_WINDOWS_TRACK_MEMORY 
		HeapTracker->AllocateEvent(address, (unsigned long) size);
#endif
	}

	void WindowsMemory::TrackDeallocation(void* address)
	{
#if ZN_WINDOWS_TRACK_MEMORY 
		HeapTracker->DeallocateEvent(address);
#endif
	}

    void* WindowsVirtualMemory::Reserve(size_t size)
    {
        return VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_READWRITE);
    }

    void* WindowsVirtualMemory::Allocate(size_t size)
    {
        return VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    }

    bool WindowsVirtualMemory::Release(void * address)
    {
        return VirtualFree(address, 0, MEM_RELEASE);
    }

    bool WindowsVirtualMemory::Commit(void * address, size_t size)
    {
        return VirtualAlloc(address, size, MEM_COMMIT, PAGE_READWRITE);
    }

#pragma warning(push)
#pragma warning(disable: 6250)		// Warning C6250 Calling 'VirtualFree' without the MEM_RELEASE flag might free memory but not address descriptors(VADs).This causes address space leaks.

    bool WindowsVirtualMemory::Decommit(void * address, size_t size)
    {
        return VirtualFree(address, size, MEM_DECOMMIT);
    }

#pragma warning(pop)

    size_t WindowsVirtualMemory::GetPageSize()
    {
        return WindowsMisc::GetSystemInfo().m_PageSize;
    }

	VirtualMemoryInformation WindowsVirtualMemory::GetMemoryInformation(void* address, size_t size)
	{
		MEMORY_BASIC_INFORMATION WinMemoryInformation;
		auto Result = VirtualQuery(address, &WinMemoryInformation, sizeof(WinMemoryInformation));
		
		_ASSERT(Result > 0 && Result == sizeof(WinMemoryInformation));

		VirtualMemoryInformation MemoryInformation;

		if (WinMemoryInformation.State == MEM_RESERVE)
		{
			MemoryInformation.m_State = VirtualMemory::State::kReserved;
		}
		else if (WinMemoryInformation.State == MEM_FREE)
		{
			MemoryInformation.m_State = VirtualMemory::State::kFree;

			MemoryInformation.m_Range = MemoryRange((void*)WinMemoryInformation.BaseAddress, 0ull);

			return MemoryInformation;
		}
		else if (WinMemoryInformation.State == MEM_COMMIT)
		{
			MemoryInformation.m_State = VirtualMemory::State::kCommitted;
		}

		MemoryInformation.m_Range = MemoryRange((void*)WinMemoryInformation.AllocationBase, WinMemoryInformation.RegionSize);

		return MemoryInformation;
	}
}