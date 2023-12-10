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
#pragma once

#include <Core/CoreTypes.h>
#include <Core/Memory/VirtualMemory.h>

namespace Zn
{
class StackAllocator
{
  public:
    void* Allocate(sizet size_, sizet alignment_ = MemoryAlignment::DefaultAlignment);

    void Free(void* address_) {};

  private:
    friend struct StackAllocationScope;
    template<typename T>
    friend struct TStackAllocator;

    static StackAllocator& Get();

    void Pop(void* prev_);

    StackAllocator();

    ~StackAllocator();

    VirtualMemoryRegion memoryRegion;

    void* top = nullptr;

    void* committed = nullptr;
};

template<typename T>
struct TStackAllocator
{
    using is_always_equal = std::true_type;
    using value_type      = T;

    constexpr TStackAllocator() = default;

    template<class U>
    constexpr TStackAllocator(const TStackAllocator<U>&) noexcept
    {
    }

    [[nodiscard]] T* allocate(sizet n_)
    {
        return (T*) StackAllocator::Get().Allocate(n_ * sizeof(value_type), alignof(value_type));
    }

    void deallocate(void*, sizet) noexcept
    {
    }
};

struct StackAllocationScope
{
    StackAllocationScope();

    ~StackAllocationScope();

    StackAllocator& Get();

  private:
    void* begin = nullptr;
};
} // namespace Zn
