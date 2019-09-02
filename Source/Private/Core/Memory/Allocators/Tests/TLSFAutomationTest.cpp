#include "Automation/AutomationTest.h"
#include "Automation/AutomationTestManager.h"
#include "Core/Log/LogMacros.h"
#include "Core/Memory/Allocators/TLSFAllocator.h"
#include <algorithm>
#include <utility>
#include <random>
#include <numeric>
#include <chrono>
#include <iostream>
#include "Core/Time/Time.h"

DECLARE_STATIC_LOG_CATEGORY(LogAutomationTest_TLSFAllocator, ELogVerbosity::Log)
DECLARE_STATIC_LOG_CATEGORY(LogAutomationTest_TLSFAllocator2, ELogVerbosity::Log)

namespace Zn::Automation
{
	class TLSFAutomationTest : public AutomationTest
	{
	public:

		TLSFAutomationTest()
			: m_Iterations(2)
			, m_MinAllocationSize(TLSFAllocator::FreeBlock::kMinBlockSize)
			, m_MaxAllocationSize(TLSFAllocator::kMaxAllocationSize)
		{
		}

		TLSFAutomationTest(size_t iterations)
			: m_Iterations(iterations)
			, m_MinAllocationSize(TLSFAllocator::FreeBlock::kMinBlockSize)
			, m_MaxAllocationSize(TLSFAllocator::kMaxAllocationSize)
		{
		}

		TLSFAutomationTest(size_t iterations, size_t min_allocation_size, size_t max_allocation_size)
			: m_Iterations(iterations)
			, m_MinAllocationSize(std::clamp<size_t>(min_allocation_size, sizeof(uintptr_t), TLSFAllocator::kMaxAllocationSize))
			, m_MaxAllocationSize(std::clamp<size_t>(max_allocation_size, m_MinAllocationSize, TLSFAllocator::kMaxAllocationSize))
		{
		}

		virtual void Execute() override
		{
			auto Allocator = TLSFAllocator();

			std::array<std::vector<void*>, 2> Pointers;

			std::array<std::vector<size_t>, 2> SizesArray;

			Pointers[0] = std::vector<void*>(m_Iterations, 0);
			Pointers[1] = std::vector<void*>(m_Iterations, 0);

			SizesArray[0] = std::vector<size_t>(m_Iterations, 0);
			SizesArray[1] = std::vector<size_t>(m_Iterations, 0);

			uint8 flip_flop = 0;

			for (int32 k = m_Iterations - 1; k >= 0; k--)
			{
				std::chrono::high_resolution_clock hrc;
				std::random_device rd;
				std::mt19937 gen(rd());
				std::uniform_int_distribution<> dis(m_MinAllocationSize, (int)m_MaxAllocationSize);

				auto& MemoryBlocks = Pointers[flip_flop];

				auto& Sizes = SizesArray[flip_flop];

				auto PreviousIterationAllocations = (k < (m_Iterations - 1)) ? &Pointers[!flip_flop] : nullptr;

				if (PreviousIterationAllocations)
				{
					std::shuffle(PreviousIterationAllocations->begin(), PreviousIterationAllocations->end(), std::default_random_engine(hrc.now().time_since_epoch().count()));
				}


				for (size_t i = 0; i < m_Iterations; ++i)
				{
					if (PreviousIterationAllocations)
					{
						Allocator.Free((*PreviousIterationAllocations)[i]);
					}

					auto Size = Memory::Align(dis(gen), sizeof(unsigned long));
					Sizes[i] = (Size);
					MemoryBlocks[i] = (Allocator.Allocate(Size));
				}
#if ZN_LOGGING
				auto total_allocated = std::accumulate(Sizes.begin(), Sizes.end(), 0);
				ZN_LOG(LogAutomationTest_TLSFAllocator, ELogVerbosity::Verbose, "Allocated %i bytes", total_allocated);

				if (auto PreviousIterationSizes = (k < (m_Iterations - 1)) ? &SizesArray[!flip_flop] : nullptr)
				{
					auto total_freed = std::accumulate(PreviousIterationSizes->begin(), PreviousIterationSizes->end(), 0);
					ZN_LOG(LogAutomationTest_TLSFAllocator, ELogVerbosity::Verbose, "Freed %i bytes", total_freed);
				}
#endif

				flip_flop = !flip_flop;
			}

			m_Result = Result::kOk;
		}	

		virtual bool ShouldQuitWhenCriticalError() const override
		{
			return false;
		}

	private:

		size_t m_Iterations;
		
		size_t m_MinAllocationSize;

		size_t m_MaxAllocationSize;
	};

	class TLSFAutomationTest2 : public AutomationTest
	{
	private:
		
		struct Allocation
		{
			void* m_Address;
			size_t	m_Size;
		};

		struct Data
		{
			// Large Memory
			static constexpr size_t kLargeMemoryAllocations		= 15000;

			static constexpr std::pair<size_t, size_t> kLargeMemoryAllocationRange = { 8192, TLSFAllocator::kMaxAllocationSize };
			
			// Frame Memory
			static constexpr size_t kFrameMemoryAllocations		= 18000;
			
			static constexpr std::pair<size_t, size_t> kFrameAllocationRange = { 8, 512 };

			static constexpr size_t kNumberOfFrames = 60 * 10;

			// Memory Spikes
			static constexpr size_t kMemorySpikesNum = kNumberOfFrames / 4;

			static constexpr std::pair<size_t, size_t> kMemorySpikeAllocationRange = { 2048, 16384 };

			//////////////////////////////////////////////////////////////////////////////////////////////////

			TLSFAllocator m_Allocator;

			std::array<Allocation, kLargeMemoryAllocations> m_LargeMemoryAllocations;
			
			std::array<Allocation, kFrameMemoryAllocations> m_LastFrameAllocations;

			std::array<Allocation, kMemorySpikesNum>		m_SpikesAllocations;
		};

		std::unique_ptr<Data> m_AllocationData;

		std::array<int, Data::kMemorySpikesNum> m_MemorySpikeFramesIndices;

		size_t m_CurrentFrameSpikeIndex;

		size_t m_NextSpikeIndex;		

	public:

		virtual void Prepare() override
		{
			m_AllocationData = std::make_unique<Data>();

			auto SpikesDistribution = CreateIntDistribution({ 0, Data::kNumberOfFrames });

			std::random_device rd;
			std::mt19937 gen(rd());

			size_t ComputedFrames = 0;

			auto IsDuplicate = [this, &ComputedFrames](const int Index) -> bool
			{
				auto Predicate = [Index](const auto& Element) { return Index == Element; };
				return std::any_of(m_MemorySpikeFramesIndices.cbegin(), m_MemorySpikeFramesIndices.cbegin() + ComputedFrames, Predicate);
			};
			
			m_MemorySpikeFramesIndices.fill(-1);

			while (ComputedFrames < Data::kMemorySpikesNum)
			{
				int Index = SpikesDistribution(gen);
				if (!IsDuplicate(Index))
				{
					m_MemorySpikeFramesIndices[ComputedFrames++] = Index;
				}
			}

			std::sort(std::begin(m_MemorySpikeFramesIndices), std::end(m_MemorySpikeFramesIndices));

			m_CurrentFrameSpikeIndex = 0;
			m_NextSpikeIndex = m_MemorySpikeFramesIndices[m_CurrentFrameSpikeIndex];
		}

		std::uniform_int_distribution<> CreateIntDistribution(const std::pair<size_t, size_t>& range)
		{
			return std::uniform_int_distribution<> (range.first, range.second);
		}
		
		template<typename Array>
		void Allocate(size_t allocation_size, size_t index, Array& storage)
		{
			auto AllocatedAddress = m_AllocationData->m_Allocator.Allocate(allocation_size);

			Allocation Block = { AllocatedAddress, allocation_size };

			storage[index] = std::move(Block);
		}

		void TryAllocateSpike(int frame)
		{
			if (frame == m_NextSpikeIndex)
			{
				auto SpikeDistribution = CreateIntDistribution({ Data::kMemorySpikeAllocationRange });
				
				std::random_device rd;
				std::mt19937 gen(rd());

				auto AllocationSize = Memory::Align(SpikeDistribution(gen), sizeof(uintptr_t));

				Allocate(AllocationSize, m_CurrentFrameSpikeIndex, m_AllocationData->m_SpikesAllocations);

				m_CurrentFrameSpikeIndex++;
				if (m_CurrentFrameSpikeIndex < m_MemorySpikeFramesIndices.size())
				{
					m_NextSpikeIndex = m_MemorySpikeFramesIndices[m_CurrentFrameSpikeIndex];
				}
				else
				{
					m_NextSpikeIndex = -1;
					m_CurrentFrameSpikeIndex = -1;
				}
			}
		}

		void AllocateLargeMemory()		// Simulate large block of memory being allocated.
		{
			std::random_device rd;
			std::mt19937 gen(rd());

			std::uniform_int_distribution<> dis = CreateIntDistribution(Data::kLargeMemoryAllocationRange);

			for (int i = 0; i < Data::kLargeMemoryAllocations; ++i)
			{
				auto AllocationSize = Memory::Align(dis(gen), sizeof(uintptr_t));

				Allocate(AllocationSize, i, m_AllocationData->m_LargeMemoryAllocations);
			}
		}

		void AllocateFrameMemory(const int frame, const int max_frames)
		{
			TryAllocateSpike(frame);

			using TFrameAllocations = decltype(Data::m_LastFrameAllocations);
			std::unique_ptr<TFrameAllocations> CurrentFrameAllocations = std::make_unique<TFrameAllocations>();

			std::uniform_int_distribution<> dis = CreateIntDistribution(Data::kFrameAllocationRange);
			
			std::chrono::high_resolution_clock hrc;
			std::shuffle(m_AllocationData->m_LastFrameAllocations.begin(), m_AllocationData->m_LastFrameAllocations.end(), std::default_random_engine(hrc.now().time_since_epoch().count()));

			std::random_device rd;
			std::mt19937 gen(rd());

			std::uniform_int_distribution<> RollDice = CreateIntDistribution({ 0, 100 });
			
			const bool CanDeallocate = frame > 0;

			int RemainingDeallocations = CanDeallocate ? m_AllocationData->m_LastFrameAllocations.size() : 0;

			for (int i = 0; i < max_frames; ++i)
			{
				if (auto Roll = RollDice(gen); CanDeallocate && Roll >= 60)
				{
					m_AllocationData->m_Allocator.Free(m_AllocationData->m_LastFrameAllocations[RemainingDeallocations - 1].m_Address);
					--RemainingDeallocations;
				}

				auto AllocationSize = Memory::Align(dis(gen), sizeof(uintptr_t));

				Allocate(AllocationSize, i, *CurrentFrameAllocations);
			}

			while (CanDeallocate && RemainingDeallocations > 0)
			{
				m_AllocationData->m_Allocator.Free(m_AllocationData->m_LastFrameAllocations[--RemainingDeallocations].m_Address);
			}

			m_AllocationData->m_LastFrameAllocations = std::move(*CurrentFrameAllocations);
		}

		virtual void Execute() override
		{
			AllocateLargeMemory();

			std::array<long long, Data::kNumberOfFrames> FrameDurations;
			
			auto SavedTime = SystemClock::now();

			for (int i = 0; i < Data::kNumberOfFrames; ++i)
			{
				AllocateFrameMemory(i, Data::kFrameMemoryAllocations);

				auto PreviousTime = SavedTime;
				SavedTime = SystemClock::now();

				FrameDurations[i] = std::chrono::duration_cast<std::chrono::milliseconds>(SavedTime - PreviousTime).count();
			}

			for (int i = 0; i < Data::kNumberOfFrames; ++i)
			{
				std::cout << "Frame [" << i << "] duration:\t" << FrameDurations[i] << "ms" << std::endl;
			}
			m_Result = Result::kOk;
		}

		virtual void Cleanup(bool bForce) override 
		{
			m_AllocationData = nullptr;

			m_MemorySpikeFramesIndices = { -1 };

			m_CurrentFrameSpikeIndex = -1;

			m_NextSpikeIndex = -1;
		}
	};
}

//DEFINE_AUTOMATION_STARTUP_TEST(TLSFAutomationTest_100, Zn::Automation::TLSFAutomationTest, 100);
//DEFINE_AUTOMATION_STARTUP_TEST(TLSFAutomationTest2_, Zn::Automation::TLSFAutomationTest2);
DEFINE_AUTOMATION_STARTUP_TEST(TLSFAutomationTest_1000, Zn::Automation::TLSFAutomationTest, 1000);
//DEFINE_AUTOMATION_STARTUP_TEST(TLSFAutomationTest_10000, Zn::Automation::TLSFAutomationTest, 10000, 4096, Zn::TLSFAllocator::kMaxAllocationSize);