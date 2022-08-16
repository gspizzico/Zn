#include <Znpch.h>
#include <Core/Memory/Allocators/ThreeWaysAllocator.h>

using namespace Zn;

constexpr size_t kSmallAllocatorVM = 2ull * (size_t) (StorageUnit::GigaByte);
constexpr size_t kMediumAllocatorVM = 128ull * (size_t) (StorageUnit::GigaByte);

constexpr size_t kLargeAllocatorPageSize = 64ull * (size_t) (StorageUnit::KiloByte);


ThreeWaysAllocator::ThreeWaysAllocator()
	: BaseAllocator()
	, m_Small(kSmallAllocatorVM)
	, m_Medium(kMediumAllocatorVM)
	, m_Large(VirtualMemory::AlignToPageSize(kLargeAllocatorPageSize))
{}

void* ThreeWaysAllocator::Malloc(size_t size, size_t alignment /*= DEFAULT_ALIGNMENT*/)
{
	if (size <= m_Small.GetMaxAllocationSize())
	{
		return m_Small.Allocate(size, alignment);
	}
	else if (size < m_Medium.MaxAllocationSize())
	{
		return m_Medium.Allocate(size, alignment);
	}

	return m_Large.Allocate(size, alignment);
}

void ThreeWaysAllocator::Free(void* ptr)
{

	if (ptr && !m_Small.Free(ptr))
	{
		if (!m_Medium.Free(ptr))
		{
			const bool Success = m_Large.Free(ptr);

			_ASSERT(Success);
		}
	}
}