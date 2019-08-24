#include "Automation/AutomationTest.h"
#include "Automation/AutomationTestManager.h"
#include "Core/Log/LogMacros.h"
#include "Core/Memory/Allocators/TLSFAllocator.h"
#include <algorithm>
#include <utility>
#include <random>
#include <numeric>

DECLARE_STATIC_LOG_CATEGORY(LogAutomationTest_TLSFAllocator, ELogVerbosity::Log)

namespace Zn::Automation
{
	class TLSFAutomationTest : public AutomationTest
	{
	public:
		
		TLSFAutomationTest()
			: m_Iterations(2)
			, m_MaxAllocationSize(TLSFAllocator::kMaxAllocationSize)
			, m_TestName(String("TLSFAutomationTest_").append(std::to_string(m_Iterations)))
		{
		}

		TLSFAutomationTest(size_t iterations, size_t max_allocation_size)
			: m_Iterations(iterations)
			, m_MaxAllocationSize(std::clamp(max_allocation_size, 128ull, TLSFAllocator::kMaxAllocationSize))
			, m_TestName(String("TLSFAutomationTest_").append(std::to_string(m_Iterations)))
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

			for (int k = m_Iterations - 1; k >= 0; k--)
			{
				std::random_device rd;
				std::mt19937 gen(rd());
				std::uniform_int_distribution<> dis(128ull, m_MaxAllocationSize);

				auto& MemoryBlocks = Pointers[flip_flop];

				auto& Sizes = SizesArray[flip_flop];

				auto PreviousIterationAllocations = (k < (m_Iterations - 1)) ? &Pointers[!flip_flop] : nullptr;

				if (PreviousIterationAllocations)
				{
					std::shuffle(PreviousIterationAllocations->begin(), PreviousIterationAllocations->end(), std::default_random_engine(5932));
				}


				for (int i = 0; i < m_Iterations; ++i)
				{
					if (PreviousIterationAllocations)
					{
						Allocator.Free((*PreviousIterationAllocations)[i]);
					}

					//auto Size = Memory::Align(dis(gen), sizeof(unsigned long));
					auto Size = Memory::Align(dis(gen), 128ull);
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

		virtual Name GetName() const override
		{
			return m_TestName;
		}

		virtual bool ShouldQuitWhenCriticalError() const override
		{
			return false;
		}

	private:

		size_t m_Iterations;

		size_t m_MaxAllocationSize;

		Name m_TestName;
	};
}

DEFINE_AUTOMATION_STARTUP_TEST(TLSFAutomationTest_100,	Zn::Automation::TLSFAutomationTest, 100, 64);
DEFINE_AUTOMATION_STARTUP_TEST(TLSFAutomationTest_1000,	Zn::Automation::TLSFAutomationTest, 1000, 64);
DEFINE_AUTOMATION_STARTUP_TEST(TLSFAutomationTest_10000,Zn::Automation::TLSFAutomationTest, 10000, 64);