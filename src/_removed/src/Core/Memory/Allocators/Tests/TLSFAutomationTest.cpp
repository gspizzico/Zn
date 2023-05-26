#include <Znpch.h>
#include "Automation/AutomationTest.h"
#include "Automation/AutomationTestManager.h"
#include "Core/Memory/Allocators/TLSFAllocator.h"
#include <algorithm>
#include <utility>
#include <random>
#include <numeric>
#include <chrono>
#include <iostream>
#include "Core/Time/Time.h"
#include <Core/Async/ThreadedJob.h>
#include <Core/Async/Thread.h>
#include <mimalloc.h>

DEFINE_STATIC_LOG_CATEGORY(LogAutomationTest_TLSFAllocator, ELogVerbosity::Log)
DEFINE_STATIC_LOG_CATEGORY(LogAutomationTest_TLSFAllocator2, ELogVerbosity::Log)

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
        VirtualMemoryRegion memory(1ull << 36ull);

        auto Allocator = TLSFAllocator(memory.Range());

        std::array<std::vector<void*>, 2> Pointers;

        std::array<std::vector<size_t>, 2> SizesArray;

        Pointers[0] = std::vector<void*>(m_Iterations, 0);
        Pointers[1] = std::vector<void*>(m_Iterations, 0);

        SizesArray[0] = std::vector<size_t>(m_Iterations, 0);
        SizesArray[1] = std::vector<size_t>(m_Iterations, 0);

        uint8 flip_flop = 0;

        for (int32 k = static_cast<int32>(m_Iterations - 1); k >= 0; k--)
        {
            std::chrono::high_resolution_clock    hrc;
            std::random_device                    rd;
            std::mt19937                          gen(rd());
            std::uniform_int_distribution<size_t> dis(m_MinAllocationSize, (int) m_MaxAllocationSize);

            auto& MemoryBlocks = Pointers[flip_flop];

            auto& Sizes = SizesArray[flip_flop];

            auto PreviousIterationAllocations = (k < (m_Iterations - 1)) ? &Pointers[!flip_flop] : nullptr;

            if (PreviousIterationAllocations)
            {
                std::shuffle(PreviousIterationAllocations->begin(),
                             PreviousIterationAllocations->end(),
                             std::default_random_engine(static_cast<unsigned long>(hrc.now().time_since_epoch().count())));
            }

            for (size_t i = 0; i < m_Iterations; ++i)
            {
                if (PreviousIterationAllocations)
                {
                    Allocator.Free((*PreviousIterationAllocations)[i]);
                }

                auto Size       = Memory::Align(dis(gen), sizeof(unsigned long));
                Sizes[i]        = (Size);
                MemoryBlocks[i] = (Allocator.Allocate(Size));
            }
#if ZN_LOGGING
            static constexpr size_t kZero           = 0;
            auto                    total_allocated = std::accumulate(Sizes.begin(), Sizes.end(), kZero);
            ZN_LOG(LogAutomationTest_TLSFAllocator, ELogVerbosity::Verbose, "Allocated %i bytes", total_allocated);

            if (auto PreviousIterationSizes = (k < (m_Iterations - 1)) ? &SizesArray[!flip_flop] : nullptr)
            {
                auto total_freed = std::accumulate(PreviousIterationSizes->begin(), PreviousIterationSizes->end(), kZero);
                ZN_LOG(LogAutomationTest_TLSFAllocator, ELogVerbosity::Verbose, "Freed %i bytes", total_freed);
            }
#endif

            flip_flop = static_cast<uint8>(!flip_flop);
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

struct TLSFTestAllocation
{
    void*  m_Address;
    size_t m_Size;
};

struct TLSFTestData
{
    // Large Memory
    static constexpr size_t kLargeMemoryAllocations = 15000;

    static constexpr std::pair<size_t, size_t> kLargeMemoryAllocationRange = {16384, TLSFAllocator::kMaxAllocationSize};

    // Frame Memory
    static constexpr size_t kFrameMemoryAllocations = 18000;

    static constexpr std::pair<size_t, size_t> kFrameAllocationRange = {256, 2048};

    static constexpr size_t kNumberOfFrames = 20 * 10;

    // Memory Spikes
    static constexpr size_t kMemorySpikesNum = kNumberOfFrames / 4;

    static constexpr std::pair<size_t, size_t> kMemorySpikeAllocationRange = {16384, 16384 * 2};

    //////////////////////////////////////////////////////////////////////////////////////////////////

    VirtualMemoryRegion m_Region = VirtualMemoryRegion(Memory::GetMemoryStatus().m_TotalPhys);
};

class IndirectTestAllocator
{
  public:
    virtual ~IndirectTestAllocator()
    {
    }
    virtual void* Allocate(sizet size) = 0;
    virtual void  Free(void* ptr)      = 0;
};

class IndirectTLSFAllocator : public IndirectTestAllocator
{
  public:
    IndirectTLSFAllocator(Zn::MemoryRange range)
        : allocator(new Zn::TLSFAllocator(range))
    {
    }

    ~IndirectTLSFAllocator()
    {
        delete allocator;
    }
    virtual void* Allocate(sizet size) override
    {
        return allocator->Allocate(size);
    }
    virtual void Free(void* ptr) override
    {
        allocator->Free(ptr);
    }

    Zn::TLSFAllocator* allocator = nullptr;
};

class IndirectMiMalloc : public IndirectTestAllocator
{
    virtual void* Allocate(sizet size) override
    {
        return mi_new(size);
    }
    virtual void Free(void* ptr) override
    {
        return mi_free(ptr);
    }
};

class TLSFAutomationThreadedJob : public ThreadedJob
{
  private:
    std::array<int, TLSFTestData::kMemorySpikesNum> m_MemorySpikeFramesIndices;

    size_t m_CurrentFrameSpikeIndex = 0;

    size_t m_NextSpikeIndex = 0;

    std::array<TLSFTestAllocation, TLSFTestData::kLargeMemoryAllocations> m_LargeMemoryAllocations {};

    std::array<TLSFTestAllocation, TLSFTestData::kFrameMemoryAllocations> m_LastFrameAllocations {};

    std::array<TLSFTestAllocation, TLSFTestData::kMemorySpikesNum> m_SpikesAllocations {};

    IndirectTestAllocator* allocator = nullptr;

  public:
    TLSFAutomationThreadedJob(IndirectTestAllocator* allocator_)
        : allocator(allocator_)
    {
    }

    void Prepare() override
    {
        auto SpikesDistribution = CreateIntDistribution({0, TLSFTestData::kNumberOfFrames});

        std::random_device rd;
        std::mt19937       gen(rd());

        size_t ComputedFrames = 0;

        auto IsDuplicate = [this, &ComputedFrames](const auto Index) -> bool
        {
            auto Predicate = [Index](const auto& Element)
            {
                return Index == Element;
            };
            return std::any_of(m_MemorySpikeFramesIndices.cbegin(), m_MemorySpikeFramesIndices.cbegin() + ComputedFrames, Predicate);
        };

        m_MemorySpikeFramesIndices.fill(-1);

        while (ComputedFrames < TLSFTestData::kMemorySpikesNum)
        {
            auto Index = SpikesDistribution(gen);
            if (!IsDuplicate(Index))
            {
                m_MemorySpikeFramesIndices[ComputedFrames++] = static_cast<int>(Index);
            }
        }

        std::sort(std::begin(m_MemorySpikeFramesIndices), std::end(m_MemorySpikeFramesIndices));

        m_CurrentFrameSpikeIndex = 0;
        m_NextSpikeIndex         = m_MemorySpikeFramesIndices[m_CurrentFrameSpikeIndex];
    }

    std::uniform_int_distribution<size_t> CreateIntDistribution(const std::pair<size_t, size_t>& range)
    {
        return std::uniform_int_distribution<size_t>(range.first, range.second);
    }

    template<typename Array>
    void Allocate(size_t allocation_size, size_t index, Array& storage)
    {
        auto AllocatedAddress = allocator->Allocate(allocation_size);

        TLSFTestAllocation Block = {AllocatedAddress, allocation_size};

        storage[index] = std::move(Block);
    }

    void TryAllocateSpike(int frame)
    {
        if (frame == m_NextSpikeIndex)
        {
            auto SpikeDistribution = CreateIntDistribution({TLSFTestData::kMemorySpikeAllocationRange});

            std::random_device rd;
            std::mt19937       gen(rd());

            auto AllocationSize = Memory::Align(SpikeDistribution(gen), sizeof(uintptr_t));

            Allocate(AllocationSize, m_CurrentFrameSpikeIndex, m_SpikesAllocations);

            m_CurrentFrameSpikeIndex++;
            if (m_CurrentFrameSpikeIndex < m_MemorySpikeFramesIndices.size())
            {
                m_NextSpikeIndex = m_MemorySpikeFramesIndices[m_CurrentFrameSpikeIndex];
            }
            else
            {
                m_NextSpikeIndex         = -1;
                m_CurrentFrameSpikeIndex = -1;
            }
        }
    }

    void AllocateLargeMemory() // Simulate large block of memory being allocated.
    {
        std::random_device rd;
        std::mt19937       gen(rd());

        std::uniform_int_distribution<size_t> dis = CreateIntDistribution(TLSFTestData::kLargeMemoryAllocationRange);

        for (int i = 0; i < TLSFTestData::kLargeMemoryAllocations; ++i)
        {
            auto AllocationSize = Memory::Align(dis(gen), sizeof(uintptr_t));

            Allocate(AllocationSize, i, m_LargeMemoryAllocations);
        }
    }

    void AllocateFrameMemory(const int frame, const int max_frames)
    {
        TryAllocateSpike(frame);

        using TFrameAllocations                                    = decltype(m_LastFrameAllocations);
        std::unique_ptr<TFrameAllocations> CurrentFrameAllocations = std::make_unique<TFrameAllocations>();

        std::uniform_int_distribution<size_t> dis = CreateIntDistribution(TLSFTestData::kFrameAllocationRange);

        std::chrono::high_resolution_clock hrc;
        std::shuffle(m_LastFrameAllocations.begin(),
                     m_LastFrameAllocations.end(),
                     std::default_random_engine(static_cast<unsigned long>(hrc.now().time_since_epoch().count())));

        std::random_device rd;
        std::mt19937       gen(rd());

        std::uniform_int_distribution<size_t> RollDice = CreateIntDistribution({0, 100});

        const bool CanDeallocate = frame > 0;

        int RemainingDeallocations = CanDeallocate ? static_cast<int>(m_LastFrameAllocations.size()) : 0;

        for (int i = 0; i < max_frames; ++i)
        {
            if (auto Roll = RollDice(gen); CanDeallocate && Roll >= 60)
            {
                allocator->Free(m_LastFrameAllocations[RemainingDeallocations - 1].m_Address);
                --RemainingDeallocations;
            }

            auto AllocationSize = Memory::Align(dis(gen), sizeof(uintptr_t));

            Allocate(AllocationSize, i, *CurrentFrameAllocations);
        }

        while (CanDeallocate && RemainingDeallocations > 0)
        {
            allocator->Free(m_LastFrameAllocations[--RemainingDeallocations].m_Address);
        }

        m_LastFrameAllocations = std::move(*CurrentFrameAllocations);
    }

    virtual void DoWork() override
    {
        ZN_TRACE_QUICKSCOPE();
        AllocateLargeMemory();

        std::array<long long, TLSFTestData::kNumberOfFrames> FrameDurations;

        auto SavedTime = SystemClock::now();

        for (int i = 0; i < TLSFTestData::kNumberOfFrames; ++i)
        {
            AllocateFrameMemory(i, TLSFTestData::kFrameMemoryAllocations);

            auto PreviousTime = SavedTime;
            SavedTime         = SystemClock::now();

            FrameDurations[i] = std::chrono::duration_cast<std::chrono::milliseconds>(SavedTime - PreviousTime).count();
        }

        // for (int i = 0; i < TLSFTestData::kNumberOfFrames; ++i)
        //{
        //     std::cout << "Frame [" << i << "] duration:\t" << FrameDurations[i] << "ms" << std::endl;
        // }
    }
};

class TLSFAutomationTestThreaded : public AutomationTest
{
  public:
    TLSFAutomationTestThreaded(u32 numThreads_, bool useMimalloc_)
        : numThreads(numThreads_)
        , useMimalloc(useMimalloc_)
    {
    }

    virtual void Prepare()
    {
        allocationData = new TLSFTestData();
        if (!useMimalloc)
        {
            allocator = new IndirectTLSFAllocator(allocationData->m_Region.Range());
        }
        else
        {
            allocator = new IndirectMiMalloc();
        }
    }

    virtual void Execute()
    {
        Vector<TLSFAutomationThreadedJob*> jobs;
        Vector<Thread*>                    threads;

        for (u32 index = 0; index < numThreads; ++index)
        {
            TLSFAutomationThreadedJob* job = new TLSFAutomationThreadedJob(allocator);
            job->Prepare();
            jobs.push_back(job);
        }

        for (u32 index = 0; index < numThreads; ++index)
        {
            Thread* thread = Thread::New(std::to_string(index), jobs[index]);
            thread->Wait(50);
            threads.push_back(thread);
        }

        for (u32 index = 0; index < numThreads; ++index)
        {
            threads[index]->WaitUntilCompletion();
            delete threads[index];
            delete jobs[index];
        }
    }

    virtual void Cleanup()
    {
        delete allocator;
        delete allocationData;
    }

    u32 numThreads = 1;

    TLSFTestData*          allocationData = nullptr;
    IndirectTestAllocator* allocator      = nullptr;
    bool                   useMimalloc    = false;
};
} // namespace Zn::Automation

DEFINE_AUTOMATION_STARTUP_TEST(TLSFAutomationTest_1000, Zn::Automation::TLSFAutomationTest, 1000);
DEFINE_AUTOMATION_STARTUP_TEST(TLSFAutomationTestThreaded_TLSF, Zn::Automation::TLSFAutomationTestThreaded, 1, false);
DEFINE_AUTOMATION_STARTUP_TEST(TLSFAutomationTestThreaded_TLSF_4, Zn::Automation::TLSFAutomationTestThreaded, 8, false);
DEFINE_AUTOMATION_STARTUP_TEST(TLSFAutomationTestThreaded_MiMalloc, Zn::Automation::TLSFAutomationTestThreaded, 1, true);
DEFINE_AUTOMATION_STARTUP_TEST(TLSFAutomationTestThreaded_MiMalloc_4, Zn::Automation::TLSFAutomationTestThreaded, 8, true);
// DEFINE_AUTOMATION_STARTUP_TEST(TLSFAutomationTest_10000, Zn::Automation::TLSFAutomationTest, 10000, 4096,
// Zn::TLSFAllocator::kMaxAllocationSize);
