#include <Core/Memory/Allocators/Strategies/TinyAllocatorStrategy.h>
#include <Core/Memory/Memory.h>

using namespace Zn;

TinyAllocatorStrategy::TinyAllocatorStrategy(size_t capacity)
	: memory_(capacity, VirtualMemory::AlignToPageSize(16 * (size_t)StorageUnit::KiloByte))
	, free_lists_()
	, num_allocations_()
	, num_free_pages_(0)
{
	std::fill(free_lists_.begin(), free_lists_.end(), nullptr);

	for (auto index = 0; index < num_allocations_.size(); ++index)
	{
		num_allocations_[index] = std::floorl(memory_.PageSize() / (16 * (index + 1)));
	}
}

void* TinyAllocatorStrategy::Allocate(size_t size, size_t alignment)
{
	//#todo handle alignment

	_ASSERT(size <= kMaxAllocationSize);

	size_t free_list_index = GetFreeListIndex(size);

	const size_t slot_size = 16 * (free_list_index + 1);

	auto& current_free_list = free_lists_[free_list_index];

	// If we don't have any page in the free list, request a new one.
	if (current_free_list == nullptr)
	{
		FreeBlock* block = new (memory_.Allocate()) FreeBlock();

		block->next_ = nullptr;
		block->size_ = std::truncl(memory_.PageSize() / slot_size) * slot_size;

		current_free_list = block;
	}

	void* allocation = static_cast<void*>(current_free_list);

	FreeBlock slot = *current_free_list;

	if (slot.next_ != nullptr)
	{
		current_free_list = slot.next_;

		if (current_free_list->next_ == nullptr)
		{
			num_free_pages_--;
		}
	}
	else
	{
		if (slot.size_ == slot_size)
		{
			current_free_list = nullptr;
		}
		else
		{
			current_free_list = new(Memory::AddOffset(allocation, slot_size)) FreeBlock();

			current_free_list->size_ = slot.size_ - slot_size;
		}
	}

	MemoryDebug::MarkUninitialized(allocation, Memory::AddOffset(allocation, slot_size));

	return allocation;
}

void TinyAllocatorStrategy::Free(void* address, size_t size)
{
	_ASSERT(memory_.IsAllocated(address));

	size_t free_list_index = GetFreeListIndex(size);

	auto& current_free_list = free_lists_[free_list_index];

	const size_t slot_size = 16 * (free_list_index + 1);

	MemoryDebug::MarkFree(address, Memory::AddOffset(address, slot_size));

	if (current_free_list == nullptr)
	{
		current_free_list = new (address) FreeBlock();


		current_free_list->next_ = nullptr;
		current_free_list->size_ = slot_size;
	}
	else
	{
		FreeBlock* next = current_free_list;
		current_free_list = new (address) FreeBlock();
		current_free_list->next_ = next;
		current_free_list->size_ = 0;

		if (next->size_ > 0)
		{
			num_free_pages_++;
		}
	}
}


size_t TinyAllocatorStrategy::GetFreeListIndex(size_t size) const
{
	return std::max(int(size >> 4) - 1, 0);
}