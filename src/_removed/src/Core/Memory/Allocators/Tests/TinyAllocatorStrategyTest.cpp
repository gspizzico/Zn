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
#include <Core/Async/Thread.h>
#include <Core/Async/ThreadedJob.h>

DEFINE_STATIC_LOG_CATEGORY(LogAutomationTest_TinyAllocatorStrategyTest, ELogVerbosity::Log)

namespace Zn::Automation
{

std::uniform_int_distribution<size_t> CreateIntDistribution(const std::pair<size_t, size_t>& range)
{
    return std::uniform_int_distribution<size_t>(range.first, range.second);
}

class TinyAllocatorStrategyTestJob : public ThreadedJob
{
  public:
    TinyAllocatorStrategyTestJob(u32 allocationSize_, u32 allocations_, u32 iterations_, Zn::TinyAllocatorStrategy* allocator_)
        : allocationSize(allocationSize_)
        , allocations(allocations_)
        , iterations(iterations_)
        , allocator(allocator_)
    {
    }

    void DoWork() override
    {
        ZN_TRACE_QUICKSCOPE();

        std::vector<std::pair<void*, size_t>> PreviousAllocations;

        std::chrono::high_resolution_clock hrc;

        for (int frame = 0; frame < iterations; frame++)
        {
            size_t ToDeallocate = 0;

            if (frame > 0)
            {
                std::random_device rd;
                std::mt19937       gen(rd());

                auto RollDice = CreateIntDistribution({allocations / 2, PreviousAllocations.size()});

                ToDeallocate += RollDice(gen);

                std::shuffle(PreviousAllocations.begin(),
                             PreviousAllocations.end(),
                             std::default_random_engine(static_cast<unsigned long>(hrc.now().time_since_epoch().count())));
            }

            std::vector<std::pair<void*, size_t>> CurrentFrameAllocations(allocations, std::pair<void*, size_t>(nullptr, 0));

            int allocation   = 0;
            int deallocation = 0;

            std::random_device rd;
            std::mt19937       gen(rd());

            auto FrameAllocationDistribution = CreateIntDistribution({sizeof(uintptr_t), 255});

            for (;;)
            {
                if (deallocation == ToDeallocate && allocation == allocations)
                    break;

                if (deallocation < ToDeallocate)
                {
                    allocator->Free(PreviousAllocations[deallocation].first);
                    deallocation++;
                }
                if (allocation < allocations)
                {
                    auto AllocationSize                 = FrameAllocationDistribution(gen);
                    auto Value                          = allocator->Allocate(AllocationSize);
                    CurrentFrameAllocations[allocation] = std::pair<void*, size_t>(Value, AllocationSize);
                    allocation++;
                }
            }

            PreviousAllocations.erase(PreviousAllocations.begin(), PreviousAllocations.begin() + ToDeallocate);

            PreviousAllocations.insert(PreviousAllocations.end(), CurrentFrameAllocations.begin(), CurrentFrameAllocations.end());
        }
    }

  private:
    u32                        allocationSize;
    u32                        allocations;
    u32                        iterations;
    Zn::TinyAllocatorStrategy* allocator;
};

class TinyAllocatorStrategyTest : public AutomationTest
{
  private:
    size_t m_AllocationSize;

    size_t m_Allocations;

    size_t m_Frames;

    VirtualMemoryRegion m_Memory;

    UniquePtr<Zn::TinyAllocatorStrategy> m_Allocator;

    u32 m_ThreadCount;

  public:
    TinyAllocatorStrategyTest(size_t allocationSize, size_t allocations, size_t frames, u32 threadCount)
        : m_AllocationSize(allocationSize)
        , m_Allocations(allocations)
        , m_Frames(frames)
        , m_Memory(m_AllocationSize)
        , m_Allocator(nullptr)
        , m_ThreadCount(threadCount)
    {
    }

    virtual void Prepare()
    {
        m_Allocator = std::make_unique<Zn::TinyAllocatorStrategy>(m_Memory.Range());
    }

    virtual void Execute()
    {
        if (m_ThreadCount > 0)
        {
            Vector<Thread*>                       threads;
            Vector<TinyAllocatorStrategyTestJob*> jobs;
            for (u32 index = 0; index < m_ThreadCount; ++index)
            {
                TinyAllocatorStrategyTestJob* job =
                    new TinyAllocatorStrategyTestJob(m_AllocationSize, m_Allocations, m_Frames, m_Allocator.get());

                Thread* thread = Thread::New(std::to_string(index), job);

                threads.push_back(thread);
                jobs.push_back(job);
            }

            for (Thread* thread : threads)
            {
                thread->WaitUntilCompletion();
                delete thread;
            }

            for (TinyAllocatorStrategyTestJob* job : jobs)
            {
                delete job;
            }

            m_Result = Result::kOk;
        }
        else
        {
            m_Result = Result::kCannotRun;
        }
    }

    virtual void Cleanup() override
    {
        AutomationTest::Cleanup();

        m_Allocator = nullptr;
    }
};
} // namespace Zn::Automation

DEFINE_AUTOMATION_STARTUP_TEST(
    TinyAllocatorTest, Zn::Automation::TinyAllocatorStrategyTest, size_t(Zn::StorageUnit::GigaByte) * 1, 250000, 2, 4);
