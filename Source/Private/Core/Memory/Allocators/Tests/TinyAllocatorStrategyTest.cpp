#include <Znpch.h>
#include "Automation/AutomationTest.h"
#include "Automation/AutomationTestManager.h"
#include "Core/Memory/Allocators/Strategies/TinyAllocatorStrategy.h"
#include <algorithm>
#include <utility>
#include <random>
#include <numeric>
#include <array>
#include <chrono>

DEFINE_STATIC_LOG_CATEGORY(LogAutomationTest_TinyAllocatorStrategyTest, ELogVerbosity::Log)

namespace Zn::Automation
{
	class TinyAllocatorStrategyTest : public AutomationTest
	{
	private:

		std::uniform_int_distribution<size_t> CreateIntDistribution(const std::pair<size_t, size_t>& range)
		{
			return std::uniform_int_distribution<size_t>(range.first, range.second);
		}

		size_t m_AllocationSize;

		size_t m_Allocations;

		size_t m_Frames;

		VirtualMemoryRegion m_Memory;

		UniquePtr<Zn::TinyAllocatorStrategy> m_Allocator;

	public:

		TinyAllocatorStrategyTest(size_t allocationSize, size_t allocations, size_t frames)
			: m_AllocationSize(allocationSize)
			, m_Allocations(allocations)
			, m_Frames(frames)
			, m_Memory(m_AllocationSize)
			, m_Allocator(nullptr)
		{}

		virtual void Prepare()
		{
			m_Allocator = std::make_unique<Zn::TinyAllocatorStrategy>(m_Memory.Range());
		}

		virtual void Execute()
		{
			std::vector<std::pair<void*, size_t>> PreviousAllocations;

			std::chrono::high_resolution_clock hrc;

			for (int frame = 0; frame < m_Frames; frame++)
			{
				size_t ToDeallocate = 0;

				if (frame > 0)
				{
					std::random_device rd;
					std::mt19937 gen(rd());

					auto RollDice = CreateIntDistribution({ m_Allocations / 2,  PreviousAllocations.size() });

					ToDeallocate += RollDice(gen);

					std::shuffle(PreviousAllocations.begin(), PreviousAllocations.end(), std::default_random_engine(static_cast<unsigned long>(hrc.now().time_since_epoch().count())));
				}

				std::vector<std::pair<void*, size_t>> CurrentFrameAllocations(m_Allocations, std::pair<void*, size_t>(nullptr, 0));

				int allocation = 0;
				int deallocation = 0;

				std::random_device rd;
				std::mt19937 gen(rd());

				auto FrameAllocationDistribution = CreateIntDistribution({ sizeof(uintptr_t), 255 });

				for (;;)
				{
					if (deallocation == ToDeallocate && allocation == m_Allocations)
						break;

					if (deallocation < ToDeallocate)
					{
						m_Allocator->Free(PreviousAllocations[deallocation].first);
						deallocation++;
					}
					if (allocation < m_Allocations)
					{
						auto AllocationSize = FrameAllocationDistribution(gen);
						auto Value = m_Allocator->Allocate(AllocationSize);
						CurrentFrameAllocations[allocation] = std::pair<void*, size_t>(Value, AllocationSize);
						allocation++;
					}
				}

				PreviousAllocations.erase(PreviousAllocations.begin(), PreviousAllocations.begin() + ToDeallocate);

				PreviousAllocations.insert(PreviousAllocations.end(), CurrentFrameAllocations.begin(), CurrentFrameAllocations.end());

			}

			m_Result = Result::kOk;
		}

		virtual void Cleanup() override
		{
			AutomationTest::Cleanup();

			m_Allocator = nullptr;
		}
	};
}

DEFINE_AUTOMATION_STARTUP_TEST(TinyAllocatorTest, Zn::Automation::TinyAllocatorStrategyTest, size_t(Zn::StorageUnit::GigaByte) * 1, 250000, 2);