#include "Automation/AutomationTest.h"
#include "Automation/AutomationTestManager.h"
#include "Core/Log/LogMacros.h"
#include "Core/Memory/Allocators/Strategies/SmallAllocationStrategy.h"
#include <algorithm>
#include <utility>
#include <random>
#include <numeric>
#include <array>
#include <chrono>

DEFINE_STATIC_LOG_CATEGORY(LogAutomationTest_SmallAllocationStrategy, ELogVerbosity::Log)

namespace Zn::Automation
{
	class SmallAllocationStrategyAutomationTest : public AutomationTest
	{
	private:

		std::uniform_int_distribution<> CreateIntDistribution(const std::pair<size_t, size_t>& range)
		{
			return std::uniform_int_distribution<>(range.first, range.second);
		}

		size_t m_Allocations;

		size_t m_Frames;

	public:

		SmallAllocationStrategyAutomationTest(size_t allocations, size_t frames)
			: m_Allocations(allocations)
			, m_Frames(frames)
		{
		}

		virtual void Execute()
		{
			Zn::SmallAllocationStrategy Strategy = SmallAllocationStrategy(1ull << 29ull, 1ull << 8ull);

			std::chrono::high_resolution_clock hrc;

			std::vector<void*> PreviousAllocations;

			for (int frame = 0; frame < m_Frames; frame++)
			{
				int ToDeallocate = 0;

				if (frame > 0)
				{
					std::random_device rd;
					std::mt19937 gen(rd());

					auto RollDice = CreateIntDistribution({ m_Allocations / 2,  PreviousAllocations.size() });

					ToDeallocate += RollDice(gen);

					std::shuffle(PreviousAllocations.begin(), PreviousAllocations.end(), std::default_random_engine(hrc.now().time_since_epoch().count()));
				}

				std::vector<void*> CurrentFrameAllocations(m_Allocations, 0);

				int allocation = 0;
				int deallocation = 0;

				std::random_device rd;
				std::mt19937 gen(rd());

				auto FrameAllocationDistribution = CreateIntDistribution({ sizeof(uintptr_t), 8/*Strategy.GetMaxAllocationSize()*/ });

				for (;;)
				{
					if (deallocation == ToDeallocate && allocation == m_Allocations)
						break;

					if (deallocation < ToDeallocate)
					{
						Strategy.Free(PreviousAllocations[deallocation]);
						deallocation++;
					}
					if (allocation < m_Allocations)
					{
						auto AllocationSize = FrameAllocationDistribution(gen);
						CurrentFrameAllocations[allocation] = Strategy.Allocate(AllocationSize);
						allocation++;
					}
				}

				PreviousAllocations.insert(PreviousAllocations.end(), CurrentFrameAllocations.begin(), CurrentFrameAllocations.end());

				PreviousAllocations.erase(PreviousAllocations.begin(), PreviousAllocations.begin() + ToDeallocate);

				ZN_LOG(LogAutomationTest_SmallAllocationStrategy, ELogVerbosity::Log, "Wasted Memory kB: %.f", float(Strategy.GetWastedMemory()) / float(StorageUnit::KiloByte));
			}

			for (auto& address : PreviousAllocations)
			{
				Strategy.Free(address);
			}

			m_Result = Result::kOk;
		}

		virtual void Cleanup(bool bForce) override
		{	
		}
	};
}

DEFINE_AUTOMATION_STARTUP_TEST(SmallAllocationStrategy, Zn::Automation::SmallAllocationStrategyAutomationTest, 5000, 60*20);