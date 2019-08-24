#include "Core/Log/Log.h"
#include "Core/Log/OutputDeviceManager.h"
#include "Core/Name.h"
#include "Core/HAL/BasicTypes.h"
#include "Core/Memory/Memory.h"
#include "Core/Windows/WindowsDebugOutput.h"
#include "Core/Log/LogMacros.h"
#include "Core/Memory/Allocators/StackAllocator.h"
#include "Core/Memory/Allocators/PoolAllocator.h"
#include "Core/Memory/Allocators/TLSFAllocator.h"
#include "Core/Memory/Allocators/HeapAllocator.h"
#include "Automation/AutomationTestManager.h"
#include "Core/HAL/Misc.h"
#include <algorithm>
#include <utility>
#include <random>
#include <numeric>

DECLARE_STATIC_LOG_CATEGORY(LogMainCpp, ELogVerbosity::Verbose);

using namespace Zn;

class Engine
{
public:

	void Initialize()
	{
		OutputDeviceManager::Get().RegisterOutputDevice<WindowsDebugOutput>();
	}

	void TestHeapAllocator()
	{
		VirtualMemoryHeap Heap(VirtualMemory::GetPageSize() * 5);
		auto Region = Heap.AllocateRegion();
		VirtualMemory::Commit(Region->Begin(), VirtualMemory::GetPageSize() * 3);

		auto Allocator = HeapAllocator(std::move(Heap), VirtualMemory::GetPageSize());
		for (int i = 0; i < 100; ++i)
		{
			Allocator.AllocatePage();
		}
	}

	void TestStackAllocator()
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
	}

	void TestFreeBlock()
	{
		static constexpr size_t kBlockSize = 32;
		MemoryPool Pool(kBlockSize);

		void* First = Pool.Allocate();
		void* Second = Pool.Allocate();
		void* Third = Pool.Allocate();

		auto first_block = new (First) TLSFAllocator::FreeBlock(kBlockSize, nullptr, nullptr);
		auto second_block = new (Second) TLSFAllocator::FreeBlock(kBlockSize, first_block, nullptr);
		auto third_block = new (Third) TLSFAllocator::FreeBlock(kBlockSize, second_block, nullptr);
		second_block->GetFooter()->m_Next = third_block;
		first_block->GetFooter()->m_Next = second_block;

		first_block->~FreeBlock();
		Pool.Free(first_block);
	}

	void TestMemoryPool()
	{
		MemoryPool Pool(1024);

		//AutoLogCategory("PoolAllocator", ELogVerbosity::Log);

		for (int reps = 10; reps > 0; --reps)
		{
			//ZN_LOG(PoolAllocator, ELogVerbosity::Log, "Loop %ull", reps);

			std::vector<void*> Pointers;
			size_t kNumAllocations = 20 * reps;
			Pointers.reserve(kNumAllocations);

			for (int index = 0; index < kNumAllocations; index++)
			{
				Pointers.emplace_back(Pool.Allocate());
				//ZN_LOG(PoolAllocator, ELogVerbosity::Log, "Init - Utilization, %.8f", Pool.GetMemoryUtilization());
			}

			std::shuffle(Pointers.begin(), Pointers.end(), std::default_random_engine(5932));

			for (int index = 0; index < kNumAllocations / 2; index++)
			{
				auto Ptr = Pointers[index];
				void* nptr = nullptr;
				std::swap(Pointers[index], nptr);
				Pool.Free(Ptr);
				//ZN_LOG(PoolAllocator, ELogVerbosity::Log, "Free - Utilization, %.8f", Pool.GetMemoryUtilization());
			}

			for (int index = 0; index < kNumAllocations; index++)
			{
				auto Ptr = Pointers[index];
				if (Ptr == nullptr)
				{
					auto _ptr = Pool.Allocate();
					std::swap(Pointers[index], _ptr);
					//ZN_LOG(PoolAllocator, ELogVerbosity::Log, "Allocate - Utilization, %.8f", Pool.GetMemoryUtilization());

				}
			}

			std::shuffle(Pointers.begin(), Pointers.end(), std::default_random_engine(5932));

			for (int index = 0; index < kNumAllocations; index++)
			{
				auto Ptr = Pointers[index];
				Pool.Free(Ptr);
				//ZN_LOG(PoolAllocator, ELogVerbosity::Log, "Free - Utilization, %.8f", Pool.GetMemoryUtilization());
			}
		}

	}
	bool DoWork()
	{
		Automation::AutomationTestManager::Get().ExecuteStartupTests();
		TestHeapAllocator();
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