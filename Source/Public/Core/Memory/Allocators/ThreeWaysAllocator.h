#pragma once

#include <Core/Memory/Memory.h>
#include <Core/Memory/Allocators/BaseAllocator.h>
#include <Core/Memory/Allocators/Strategies/TinyAllocatorStrategy.h>
#include <Core/Memory/Allocators/TLSFAllocator.h>
#include <Core/Memory/Allocators/Strategies/DirectAllocationStrategy.h>

namespace Zn
{
	class ThreeWaysAllocator : public BaseAllocator
	{
	public:

		ThreeWaysAllocator();

		virtual ~ThreeWaysAllocator() = default;

		virtual void* Malloc(size_t size, size_t alignment = DEFAULT_ALIGNMENT) override;

		virtual bool Free(void* ptr) override;

		//virtual void* Realloc(void* ptr, size_t size, size_t alignment = DEFAULT_ALIGNMENT) = 0;

	private:

		VirtualMemoryRegion region;

		TinyAllocatorStrategy m_Small;
		TLSFAllocator m_Medium;
		DirectAllocationStrategy m_Large;
	};
}