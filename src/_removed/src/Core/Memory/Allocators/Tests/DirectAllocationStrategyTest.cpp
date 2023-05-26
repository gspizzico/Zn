#include <Znpch.h>
#include "Automation/AutomationTest.h"
#include "Automation/AutomationTestManager.h"
#include "Core/Memory/Memory.h"
#include "Core/Memory/Allocators/Strategies/DirectAllocationStrategy.h"
#include <algorithm>
#include <utility>
#include <random>
#include <numeric>
#include <array>
#include <chrono>

DEFINE_STATIC_LOG_CATEGORY(LogAutomationTest_DirectAllocationStrategy, ELogVerbosity::Log)

namespace Zn::Automation
{
class DirectAllocationStrategyAutomationTest : public AutomationTest
{
  private:
    std::uniform_int_distribution<size_t> CreateIntDistribution(const std::pair<size_t, size_t>& range)
    {
        return std::uniform_int_distribution<size_t>(range.first, range.second);
    }

    size_t m_Allocations;

    size_t m_Frames;

  public:
    DirectAllocationStrategyAutomationTest(size_t allocations, size_t frames)
        : m_Allocations(allocations)
        , m_Frames(frames)
    {
    }

    virtual void Execute()
    {
        Zn::DirectAllocationStrategy Strategy = DirectAllocationStrategy(uint64_t(StorageUnit::KiloByte) * 64ull);

        std::chrono::high_resolution_clock hrc;

        std::vector<void*> PreviousAllocations;

        for (int frame = 0; frame < m_Frames; frame++)
        {
            size_t ToDeallocate = 0;

            if (frame > 0)
            {
                std::random_device rd;
                std::mt19937       gen(rd());

                auto RollDice = CreateIntDistribution({m_Allocations / 2, PreviousAllocations.size()});

                ToDeallocate += RollDice(gen);

                std::shuffle(
                    PreviousAllocations.begin(), PreviousAllocations.end(),
                    std::default_random_engine(static_cast<unsigned long>(hrc.now().time_since_epoch().count())));
            }

            std::vector<void*> CurrentFrameAllocations(m_Allocations, 0);

            int allocation   = 0;
            int deallocation = 0;

            std::random_device rd;
            std::mt19937       gen(rd());

            auto FrameAllocationDistribution = CreateIntDistribution({size_t(StorageUnit::KiloByte) * 64, size_t(StorageUnit::MegaByte) * 4});

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
                    auto AllocationSize                 = FrameAllocationDistribution(gen);
                    CurrentFrameAllocations[allocation] = Strategy.Allocate(AllocationSize);
                    allocation++;
                }
            }

            PreviousAllocations.insert(PreviousAllocations.end(), CurrentFrameAllocations.begin(), CurrentFrameAllocations.end());

            PreviousAllocations.erase(PreviousAllocations.begin(), PreviousAllocations.begin() + ToDeallocate);
        }

        for (auto& address : PreviousAllocations)
        {
            Strategy.Free(address);
        }

        m_Result = Result::kOk;
    }
};
} // namespace Zn::Automation

DEFINE_AUTOMATION_STARTUP_TEST(DirectAllocationStrategy, Zn::Automation::DirectAllocationStrategyAutomationTest, 500, 1);