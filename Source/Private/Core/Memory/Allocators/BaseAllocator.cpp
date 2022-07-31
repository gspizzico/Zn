#include <Core/Memory/Allocators/BaseAllocator.h>
#include <memory>

using namespace Zn;

void* SystemAllocator::operator new(size_t size)
{
	return malloc(size);
}

void SystemAllocator::operator delete(void* ptr)
{
	free(ptr);
}

void* SystemAllocator::operator new[](size_t size)
{
	return malloc(size);
}

void SystemAllocator::operator delete[](void* ptr)
{
	return free(ptr);
}
