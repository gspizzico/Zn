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
#include <Core/Containers/BitArray.h>
#include <RHI/RHIResource.h>
#include <cassert>

namespace Zn
{
template<typename T>
concept HandleType = std::derived_from<T, ResourceHandle> && !std::is_same_v<T, ResourceHandle>;

template<typename T>
concept ValueType = std::movable<T> && std::copyable<T>;

template<ValueType T, HandleType H, sizet N>
class RHIResourceBuffer
{
    using GenerationType = decltype(ResourceHandle::gen);
    using IndexType      = decltype(ResourceHandle::index);

  public:
    RHIResourceBuffer()
        : freeList()
    {
        Memory::Memzero(buffer);
        Memory::Memzero(generations);
    }

    ~RHIResourceBuffer()
    {
        Clear();
    }

    RHIResourceBuffer(const RHIResourceBuffer&)            = delete;
    RHIResourceBuffer(RHIResourceBuffer&&)                 = delete;
    RHIResourceBuffer& operator=(const RHIResourceBuffer&) = delete;
    RHIResourceBuffer& operator=(RHIResourceBuffer&&)      = delete;

    T* operator[](const H& handle_)
    {
        RangeCheck(handle_);

        if (GenerationCheck(handle_))
        {
            return &buffer[handle_.index];
        }
        else
        {
            return nullptr;
        }
    }

    const T* operator[](const H& handle_) const
    {
        return const_cast<RHIResourceBuffer*>(this)->operator[](handle_);
    }

    H Add(T&& entry_)
    {
        IndexType nextFreeSlot = GetNextFreeSlot();

        check(nextFreeSlot < N);

        freeList.Set(nextFreeSlot, 1);

        const GenerationType& gen = generations[nextFreeSlot];

        ResourceHandle handle {
            .gen   = gen,
            .index = nextFreeSlot,
        };

        T& slot = buffer[nextFreeSlot];

        slot = std::move(entry_);

        // Construct handle from basic entry type. HandleType concept enforces it.
        return H(std::move(handle));
    }

    bool Evict(const H& handle_)
    {
        RangeCheck(handle_);

        GenerationType& gen = generations[handle_.index];

        if (gen == handle_.gen)
        {
            DestroyElement(handle_.index);

            if (gen < std::numeric_limits<IndexType>::max())
            {
                ++gen;

                freeList.Set(handle_.index, 0);
            }
            // else : Slot has become unusable. Don't update nextFreeSlot.

            return true;
        }

        return false;
    }

    void Clear()
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            for (IndexType index = 0; index < N; ++index)
            {
                if (!freeList[index])
                {
                    DestroyElement(index);
                }
            }
        }

        Memory::Memzero(buffer);
        Memory::Memzero(generations);
        freeList.Reset();
    }

    struct EntryIterator
    {
        const BitArray<N>& freeList;
        T*                 ptr;
        IndexType          index;

        EntryIterator(const BitArray<N>& freeList_, T* ptr_, IndexType index_)
            : freeList(freeList_)
            , ptr(ptr_)
            , index(index_)
        {
        }

        EntryIterator& operator++()
        {
            do
            {
                ++index;
            } while (freeList[index] == false && index < N);

            return *this;
        }

        // TODO:
        bool operator!=(const EntryIterator& other_)
        {
            return index != other_.index && ptr != other_.ptr;
        }

        auto& operator*() const
        {
            return *Memory::AddOffset(ptr, sizeof(T) + index);
        }
    };

    EntryIterator begin()
    {
        return EntryIterator(freeList, DataPtr(buffer), 0);
    }

    EntryIterator end()
    {
        return EntryIterator(freeList, DataPtr(buffer), N);
    }

  private:
    bool RangeCheck(const H& handle_) const
    {
        const bool valid = handle_.index >= 0 && handle_.index < N;
        assert(valid);
        return valid;
    }

    bool GenerationCheck(const H& handle_) const
    {
        const bool valid = handle_.gen > 0 && generations[handle_.index] == handle_.gen;
        return valid;
    }

    void DestroyElement(IndexType index_)
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            T& entry = reinterpret_cast<T&>(buffer[index_]);
            entry.~T();
        }
    }

    IndexType GetNextFreeSlot() const
    {
        return (IndexType) freeList.CountReverseOne();
    }

    alignas(MemoryAlignment::DefaultAlignment) GenerationType generations[N];
    alignas(MemoryAlignment::DefaultAlignment) T buffer[N];
    alignas(MemoryAlignment::DefaultAlignment) BitArray<N> freeList;
};
} // namespace Zn
