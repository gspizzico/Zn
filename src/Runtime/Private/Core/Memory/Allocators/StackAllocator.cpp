/*
MIT License

Copyright (c) 2023 Giuseppe Spizzico

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include <Core/Memory/Allocators/StackAllocator.h>
#include <Core/Core.hpp>
#include <Core/CoreAssert.h>

Zn::StackAllocator& Zn::StackAllocator::Get()
{
    static thread_local StackAllocator tlInstance;
    return tlInstance;
}

Zn::StackAllocator::StackAllocator()
    : memoryRegion(512 * std::underlying_type_t<StorageUnit>(StorageUnit::MegaByte))
{
    top = memoryRegion.Begin();

    const bool result = VirtualMemory::Commit(memoryRegion.Begin(), VirtualMemory::GetPageSize());
    check(result);
    committed = Memory::AddOffset(memoryRegion.Begin(), VirtualMemory::GetPageSize());
}

Zn::StackAllocator::~StackAllocator()
{
    VirtualMemory::Decommit(memoryRegion.Begin(), Memory::GetDistance(committed, memoryRegion.Begin()));
}

void* Zn::StackAllocator::Allocate(sizet size_, sizet alignment_)
{
    sizet allocationSize = Memory::Align(size_, alignment_);

    sizet availableSize = Memory::GetDistance(committed, top);
    if (availableSize < allocationSize)
    {
        sizet committedSize = VirtualMemory::AlignToPageSize(allocationSize - availableSize);

        if (!VirtualMemory::Commit(committed, committedSize))
        {
            check(false);
        }
        committed = Memory::AddOffset(committed, committedSize);
    }

    void* address = top;
    top           = Memory::AddOffset(top, allocationSize);
    return address;
}

void Zn::StackAllocator::Pop(void* prev_)
{
    if (memoryRegion.Range().Contains(prev_) && prev_ < top)
    {
        MemoryDebug::MarkFree(prev_, top);
        top = prev_;
    }
}

Zn::StackAllocationScope::StackAllocationScope()
    : begin(StackAllocator::Get().top)
{
}

Zn::StackAllocationScope::~StackAllocationScope()
{
    StackAllocator::Get().Pop(begin);
}

Zn::StackAllocator& Zn::StackAllocationScope::Get()
{
    return StackAllocator::Get();
}
