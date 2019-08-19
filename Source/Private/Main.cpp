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
#include "Core/HAL/Misc.h"
#include <algorithm>
#include <utility>
#include <random>

using namespace Zn;

class Engine
{
public:
    
    void Initialize()
    {
        OutputDeviceManager::Get().RegisterOutputDevice<WindowsDebugOutput>();
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

	void TestTLSFAllocator()
	{
		//AutoLogCategory("TestTLSFAllocator", ELogVerbosity::Log);

		constexpr size_t kReps = 2000;
		auto Allocator = TLSFAllocator(8192 * kReps);
		std::array<std::vector<void*>, 2> Pointers;

		std::array<std::vector<size_t>, 2> SizesArray;

		/*for (int k = 0; k < kReps; k++)
		{
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<> dis(128, 4096);

			auto& MemoryBlocks = Pointers[k];
			auto& Sizes = SizesArray[k];
			MemoryBlocks.reserve(kReps);
			Sizes.reserve(kReps);

			for (int i = 0; i < kReps; ++i)
			{
				auto Size = Memory::Align(dis(gen), sizeof(unsigned long));
				Sizes.emplace_back(Size);
				MemoryBlocks.emplace_back(Allocator.Allocate(Size));
			}

			for (int i = 0; i < kReps; ++i)
			{
				auto& ptr = MemoryBlocks[i];
				Allocator.Free(ptr);
			}
		}*/

		Pointers[0] = std::vector<void*>(kReps, 0);
		Pointers[1] = std::vector<void*>(kReps, 0);

		SizesArray[0] = std::vector<size_t>(kReps, 0);
		SizesArray[1] = std::vector<size_t>(kReps, 0);

		uint8 flip_flop = 0;

		//for (int k = 0; k < kReps; k++)
		for (int k = kReps - 1; k >= 0 ; k--)
		{
			//std::vector<void*> PreviousIterationAllocations = k > 0 ? Pointers[k - 1] : std::vector<void*>();
			
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<> dis(std::clamp(128ull * (unsigned long long) k, 128ull, 4096ull), 4096ull);
			
			auto& MemoryBlocks = Pointers[flip_flop];

			auto& Sizes = SizesArray[flip_flop];
			
			auto PreviousIterationAllocations = (k < (kReps - 1)) ? &Pointers[!flip_flop] : nullptr;

			if (PreviousIterationAllocations)
			{
				std::shuffle(PreviousIterationAllocations->begin(), PreviousIterationAllocations->end(), std::default_random_engine(5932));
			}
			

			for (int i = 0; i < kReps; ++i)
			{
				//if (k > 0)
				if (PreviousIterationAllocations)
				{
					Allocator.Free((*PreviousIterationAllocations)[i]);
				}

				auto Size = Memory::Align(dis(gen), sizeof(unsigned long));
				Sizes[i] = (Size);
				MemoryBlocks[i] = (Allocator.Allocate(Size));
			}
			flip_flop = !flip_flop;
		}
		
		
		/*for (auto size = 32; size < 1024; size = size + 2)
		{
			TLSFAllocator::index_type fl = 0, sl = 0;
			Allocator.MappingInsert(size, fl, sl);
			Allocator.FindSuitableBlock(fl, sl);
		}*/

		/*Allocator.MappingSearch(64, fl, sl);

		Allocator.MappingInsert(8, fl, sl);
		Allocator.MappingSearch(8, fl, sl);

		Allocator.MappingInsert(128, fl, sl);
		Allocator.MappingSearch(128, fl, sl);

		Allocator.MappingInsert(1024, fl, sl);
		Allocator.MappingSearch(1024, fl, sl);

		Allocator.MappingInsert(256, fl, sl);
		Allocator.MappingSearch(256, fl, sl);

		Allocator.MappingInsert(270, fl, sl);
		Allocator.MappingSearch(270, fl, sl);*/

	}

	void TestFreeBlock()
	{
		static constexpr size_t kBlockSize = 32;
		MemoryPool Pool(kBlockSize);

		void* First  = Pool.Allocate();
		void* Second = Pool.Allocate();
		void* Third	 = Pool.Allocate();

		auto first_block =	new (First) TLSFAllocator::FreeBlock(kBlockSize, nullptr, nullptr);
		auto second_block = new (Second) TLSFAllocator::FreeBlock(kBlockSize, first_block, nullptr);
		auto third_block =	new (Third) TLSFAllocator::FreeBlock(kBlockSize, second_block, nullptr);
		second_block->GetFooter()->m_Next = third_block;
		first_block->GetFooter()->m_Next = second_block;

		first_block->~FreeBlock();
		Pool.Free(first_block);
	}

	void TestMemoryPool()
	{
		MemoryPool Pool(1024);

		AutoLogCategory("PoolAllocator", ELogVerbosity::Log);

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
		auto m = Memory::GetMemoryStatus();
		TestTLSFAllocator();
		//TestMemoryPool();
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