#include <Memory/VirtualMemory.h>
#include <CoreAssert.h>
#include <CorePlatform.h>
#include <algorithm>

DEFINE_LOG_CATEGORY(LogMemory, Zn::ELogVerbosity::Warning)

namespace Zn
{
void* VirtualMemory::Reserve(sizet size_)
{
    return PlatformVirtualMemory::Reserve(size_);
}
void* VirtualMemory::Allocate(sizet size_)
{
    return PlatformVirtualMemory::Allocate(size_);
}
bool VirtualMemory::Release(void* address_)
{
    return PlatformVirtualMemory::Release(address_);
}
bool VirtualMemory::Commit(void* address_, sizet size_)
{
    check(Memory::GetMemoryStatus().availPhys >= size_);

    if (Memory::GetMemoryStatus().availPhys < size_)
    {
        abort(); // TODO: OOM
    }
    return PlatformVirtualMemory::Commit(address_, size_);
}
bool VirtualMemory::Decommit(void* address_, sizet size_)
{
    return PlatformVirtualMemory::Decommit(address_, size_);
}
sizet VirtualMemory::GetPageSize()
{
    return PlatformVirtualMemory::GetPageSize();
}
sizet VirtualMemory::AlignToPageSize(sizet size_)
{
    return Memory::Align(size_, GetPageSize());
}

VirtualMemoryInformation VirtualMemory::GetMemoryInformation(void* address_, sizet size_)
{
    return PlatformVirtualMemory::GetMemoryInformation(address_, size_);
}

VirtualMemoryInformation VirtualMemory::GetMemoryInformation(MemoryRange range_)
{
    return VirtualMemory::GetMemoryInformation(range_.begin, range_.Size());
}

VirtualMemoryRegion::VirtualMemoryRegion(size_t capacity_)
    : range(VirtualMemory::Reserve(VirtualMemory::AlignToPageSize(capacity_)), Memory::Align(capacity_, VirtualMemory::GetPageSize()))
{
}

VirtualMemoryRegion::VirtualMemoryRegion(VirtualMemoryRegion&& other_) noexcept
    : range(std::move(other_.range))
{
}

VirtualMemoryRegion::VirtualMemoryRegion(MemoryRange range_) noexcept
    : range(range_)
{
    check(VirtualMemory::GetMemoryInformation(range).state == VirtualMemory::State::kReserved);
}

VirtualMemoryRegion::~VirtualMemoryRegion()
{
    if (auto baseAddress = range.begin)
    {
        VirtualMemory::Release(baseAddress);
    }
}
} // namespace Zn
