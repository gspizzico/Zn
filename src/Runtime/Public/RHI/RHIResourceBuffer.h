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

    // TODO: do we need two different functions?
    H Add(const T& entry_)
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

        slot = entry_;

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

    sizet AvailableSlots() const
    {
        return N - freeList.Count();
    }

    // Iterates over occupied entries.
    struct EntryIterator
    {
        EntryIterator(RHIResourceBuffer* buffer_, sizet index_)
            : buffer(buffer_)
            , index(index_)
        {
            if (index < N && buffer->freeList[index] == false)
            {
                this->operator++();
            }
        }

        EntryIterator& operator++()
        {
            do
            {
                ++index;
            } while (index < N && buffer->freeList[index] == false);

            return *this;
        }

        EntryIterator& operator--()
        {
            do
            {
                --index;
            } while (index > 0 && buffer->freeList[index] == false);

            return *this;
        }

        bool operator==(const EntryIterator& other_) const
        {
            return buffer == other_.buffer && index == other_.index;
        }

        bool operator!=(const EntryIterator& other_) const
        {
            return !(this->operator==(other_));
        }

        T& operator*() const
        {
            return buffer->buffer[index];
        }

        T* operator->() const
        {
            return &buffer->buffer[index];
        }

        void Evict()
        {
            ResourceHandle handle {
                .gen   = buffer->generations[index],
                .index = static_cast<IndexType>(index),
            };

            buffer->Evict(H(std::move(handle)));
        }

      private:
        RHIResourceBuffer* buffer;
        sizet              index;
    };

    EntryIterator begin()
    {
        return EntryIterator(this, 0);
    }

    EntryIterator end()
    {
        return EntryIterator(this, N);
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
        const bool valid = generations[handle_.index] == handle_.gen;
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
