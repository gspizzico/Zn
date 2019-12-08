#include "Automation/AutomationTest.h"
#include "Automation/AutomationTestManager.h"
#include "Core/Memory/Allocators/MultipoolAllocator.h"
#include <random>
#include <array>
#include <numeric>

DEFINE_STATIC_LOG_CATEGORY(LogAutomationTest_MultipoolAllocator, ELogVerbosity::Log)


namespace Zn::Automation
{
	class MultipoolAutomationTest : public AutomationTest
	{
	public:

		MultipoolAutomationTest(size_t iterations, size_t pools_num, size_t pool_address_space, size_t min_allocation_size)
			: m_Iterations(iterations)
			, m_PoolsNum(pools_num)
			, m_PoolAddressSpace(pool_address_space)
			, m_MinAllocationSize(min_allocation_size)
		{
		}

		virtual bool ShouldQuitWhenCriticalError() const override
		{
			return false;
		}

		virtual void Prepare() override
		{
			m_Allocator = std::make_unique<MultipoolAllocator>(MultipoolAllocator(m_PoolsNum, m_PoolAddressSpace, m_MinAllocationSize));
		}

		virtual void Cleanup(bool bForce) override
		{
			m_Allocator.reset();
		}

		virtual void Execute() override
		{
			auto MaxAllocationSize = m_Allocator->GetMaxAllocationSize();
			auto Distribution = std::uniform_int_distribution<>(m_MinAllocationSize, MaxAllocationSize);

			std::array<std::vector<void*>, 2> Pointers;
			std::array<std::vector<size_t>, 2> SizesArray;

			Pointers[0] = std::vector<void*>(m_Iterations, 0);
			Pointers[1] = std::vector<void*>(m_Iterations, 0);

			SizesArray[0] = std::vector<size_t>(m_Iterations, 0);
			SizesArray[1] = std::vector<size_t>(m_Iterations, 0);

			bool flip_flop = 0;
			for (int32 k = m_Iterations - 1; k >= 0; k--)
			{
				std::chrono::high_resolution_clock hrc;
				std::random_device rd;
				std::mt19937 gen(rd());
				std::uniform_int_distribution<> dis(m_MinAllocationSize, (int) MaxAllocationSize);

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
						auto Address = (*PreviousIterationAllocations)[i];
						if (Address != nullptr)
						{
							m_Allocator->Free((*PreviousIterationAllocations)[i]);
						}
					}

					auto Size = Memory::Align(dis(gen), sizeof(unsigned long));
					Sizes[i] = (Size);
					MemoryBlocks[i] = (m_Allocator->Allocate(Size));
				}
#if ZN_LOGGING
				auto total_allocated = std::accumulate(Sizes.begin(), Sizes.end(), 0);
				ZN_LOG(LogAutomationTest_MultipoolAllocator, ELogVerbosity::Verbose, "Allocated %i bytes", total_allocated);

				if (auto PreviousIterationSizes = (k < (m_Iterations - 1)) ? &SizesArray[!flip_flop] : nullptr)
				{
					auto total_freed = std::accumulate(PreviousIterationSizes->begin(), PreviousIterationSizes->end(), 0);
					ZN_LOG(LogAutomationTest_MultipoolAllocator, ELogVerbosity::Verbose, "Freed %i bytes", total_freed);
				}
#endif

				flip_flop = !flip_flop;
			}

			m_Result = Result::kOk;
		}

	private:

		UniquePtr<MultipoolAllocator> m_Allocator;

		size_t m_Iterations;

		size_t m_PoolsNum;

		size_t m_PoolAddressSpace;

		size_t m_MinAllocationSize;
	};
}

DEFINE_AUTOMATION_STARTUP_TEST(MultipoolAutomationTest, Zn::Automation::MultipoolAutomationTest, 500, 5, (1ull << 32ull) * 2, 1ull << 16ull);