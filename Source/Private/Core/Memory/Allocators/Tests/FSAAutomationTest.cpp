#include "Automation/AutomationTest.h"
#include "Automation/AutomationTestManager.h"
#include "Core/Log/LogMacros.h"
#include "Core/Memory/Allocators/FixedSizeAllocator.h"
#include <algorithm>
#include <utility>
#include <random>
#include <numeric>
#include <array>
#include <chrono>

DECLARE_STATIC_LOG_CATEGORY(LogAutomationTest_FSAAutomationTest, ELogVerbosity::Log)

namespace Zn::Automation
{
	class FSAAutomationTest : public AutomationTest
	{
	private:

		std::uniform_int_distribution<> CreateIntDistribution(const std::pair<size_t, size_t>& range)
		{
			return std::uniform_int_distribution<>(range.first, range.second);
		}

		size_t m_AllocationSize;
		
		size_t m_PageSize;

		size_t m_Allocations;
		
		size_t m_Frames;

		UniquePtr<Zn::FixedSizeAllocator> m_Allocator;

	public:

		FSAAutomationTest(size_t allocationSize, size_t pageSize, size_t allocations, size_t frames)
			: m_AllocationSize(allocationSize)
			, m_PageSize(pageSize)
			, m_Allocations(allocations)
			, m_Frames(frames)
			, m_Allocator(nullptr)
		{
		}

		virtual void Prepare()
		{
			m_Allocator = std::make_unique<Zn::FixedSizeAllocator>(m_AllocationSize, m_PageSize);
		}

		virtual void Execute()
		{
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

					//std::shuffle(PreviousAllocations.begin(), PreviousAllocations.end(), std::default_random_engine(hrc.now().time_since_epoch().count()));
				}

				std::vector<void*> CurrentFrameAllocations(m_Allocations, 0);
				
				int allocation = 0;
				int deallocation = 0;

				for(;;)
				{
					if (deallocation == ToDeallocate && allocation == m_Allocations)
						break;

					if (deallocation < ToDeallocate)
					{
						m_Allocator->Free(PreviousAllocations[deallocation]);
						deallocation++;
					}
					if (allocation < m_Allocations)
					{
						CurrentFrameAllocations[allocation] = m_Allocator->Allocate();
						allocation++;
					}
				}

				PreviousAllocations.insert(PreviousAllocations.end(), CurrentFrameAllocations.begin(), CurrentFrameAllocations.end());
				
				PreviousAllocations.erase(PreviousAllocations.begin(), PreviousAllocations.begin() + ToDeallocate);
			}

			for (auto& address : PreviousAllocations)
			{
				m_Allocator->Free(address);
			}

			m_Result = Result::kOk;
		}

		virtual void Cleanup(bool bForce) override
		{
			m_Allocator = nullptr;
		}
	};
}

DEFINE_AUTOMATION_STARTUP_TEST(FSAAutomationTest_8, Zn::Automation::FSAAutomationTest, 8,  1 << 14, 3000, 600);
//DEFINE_AUTOMATION_STARTUP_TEST(FSAAutomationTest_16, Zn::Automation::FSAAutomationTest, 16, 1 << 14, 3000, 600);
DEFINE_AUTOMATION_STARTUP_TEST(FSAAutomationTest_24, Zn::Automation::FSAAutomationTest, 24, 1 << 14, 3000, 600);
//DEFINE_AUTOMATION_STARTUP_TEST(FSAAutomationTest_32, Zn::Automation::FSAAutomationTest, 32, 1 << 14, 3000, 600);
//DEFINE_AUTOMATION_STARTUP_TEST(FSAAutomationTest_38, Zn::Automation::FSAAutomationTest, 38, 1 << 14, 3000, 600);