#include "Automation/AutomationTest.h"
#include "Automation/AutomationTestManager.h"
#include "Core/Log/LogMacros.h"
#include "Core/Memory/Allocators/StackAllocator.h"
#include <algorithm>
#include <utility>
#include <random>
#include <numeric>
#include <array>
#include <chrono>

DEFINE_STATIC_LOG_CATEGORY(LogAutomationTest_StackAllocatorAutomationTest, ELogVerbosity::Log)

namespace Zn::Automation
{
	class StackAllocatorAutomationTest : public AutomationTest
	{
	private:

		std::uniform_int_distribution<size_t> CreateIntDistribution(const std::pair<size_t, size_t>& range)
		{
			return std::uniform_int_distribution<size_t>(range.first, range.second);
		}

		size_t m_AllocationSize;

		size_t m_Allocations;

		size_t m_Frames;

		UniquePtr<Zn::StackAllocator> m_Allocator;

	public:

		StackAllocatorAutomationTest(size_t allocationSize, size_t allocations, size_t frames)
			: m_AllocationSize(allocationSize)
			, m_Allocations(allocations)
			, m_Frames(frames)
			, m_Allocator(nullptr)
		{}

		virtual void Prepare()
		{
			m_Allocator = std::make_unique<Zn::StackAllocator>(m_AllocationSize);
		}

		virtual void Execute()
		{
			std::vector<void*> PreviousAllocations;

			for (int frame = 0; frame < m_Frames; frame++)
			{
				size_t ToDeallocate = 0;

				if (frame > 0)
				{
					std::random_device rd;
					std::mt19937 gen(rd());

					auto RollDice = CreateIntDistribution({ m_Allocations / 2,  PreviousAllocations.size() });

					ToDeallocate += RollDice(gen);
				}

				std::random_device rd;
				std::mt19937 gen(rd());

				auto FrameAllocationDistribution = CreateIntDistribution({ size_t(StorageUnit::Byte) * 256, size_t(StorageUnit::KiloByte) * 16 });

				std::vector<void*> CurrentFrameAllocations(m_Allocations, 0);

				if (PreviousAllocations.size() > 0)
				{
					size_t deallocated = 0;
					for (auto It = PreviousAllocations.rbegin(); It != PreviousAllocations.rend() && deallocated < ToDeallocate; ++It, deallocated++)
					{
						m_Allocator->Free(*It);
						++deallocated;
					}

					PreviousAllocations.erase(PreviousAllocations.end() - deallocated, PreviousAllocations.end());
				}

				int allocation = 0;

				size_t save_allocation = 0;

				if (frame == m_Frames - 1)
				{
					auto RollDice = CreateIntDistribution({ 1, m_Allocations - 1 });

					save_allocation = RollDice(gen);
				}

				while (allocation < m_Allocations)
				{
					CurrentFrameAllocations[allocation] = m_Allocator->Allocate(FrameAllocationDistribution(gen));

					if (allocation == save_allocation)
					{
						m_Allocator->SaveStatus();
					}
					allocation++;
				}

				if (save_allocation != 0)
				{
					m_Allocator->RestoreStatus();
				}

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

DEFINE_AUTOMATION_STARTUP_TEST(StackAllocatorTest, Zn::Automation::StackAllocatorAutomationTest, size_t(Zn::StorageUnit::GigaByte) * 1, 200, 4);