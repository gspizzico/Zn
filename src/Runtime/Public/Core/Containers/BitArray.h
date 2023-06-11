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

#include "Core/CoreTypes.h"
#include "Core/CoreAssert.h"
#include <limits.h>
#include <algorithm>

namespace Zn
{
template<sizet N>
class BitArray
{
  public:
    static constexpr sizet  kNumBits   = sizeof(uint64) * CHAR_BIT;
    static constexpr uint64 kNumChunks = (kNumBits + N - 1) / kNumBits;

    BitArray()
    {
        Memory::Memzero(chunks);
    }

    BitArray(bool value_)
    {
        Memory::Memset(chunks, value_ ? 0xFF : 0x00);

        if constexpr (N < kNumBits)
        {
            const uint64 mask = ~(1ull << (N + 1));
            chunks[0] &= mask;
        }
    }

    BitArray(const BitArray& other_)
    {
        memcpy(DataPtr(chunks), DataPtr(other_.chunks), sizeof(chunks));
    }

    BitArray(BitArray&& other_)
    {
        memmove(DataPtr(chunks), DataPtr(other_.chunks), sizeof(chunks));
    }

    // Constructs from a bit array of a different size.
    // Will always copy the least significants bits.
    template<sizet K>
    BitArray(const BitArray<K>& other_)
        : BitArray()
    {
        using OtherBitArray = BitArray<K>;

        if constexpr (kNumChunks >= OtherBitArray::kNumChunks)
        {
            constexpr sizet startingChunk = kNumChunks - OtherBitArray::kNumChunks;
            memcpy(&chunks[startingChunk], &other_.chunks[0], sizeof(other_.chunks));
        }
        else
        {
            constexpr sizet sourceChunk = OtherBitArray::kNumChunks - kNumChunks;
            memcpy(&chunks[0], &other_.chunks[sourceChunk], sizeof(chunks));
        }

        if constexpr (N < kNumBits)
        {
            const uint64 mask = ~(1ull << (N + 1));
            chunks[0] &= mask;
        }
    }

    BitArray& operator=(const BitArray& other_)
    {
        memcpy(DataPtr(chunks), DataPtr(other_.chunks), sizeof(chunks));
        return *this;
    }

    BitArray& operator=(BitArray&& other_)
    {
        memmove(DataPtr(chunks), DataPtr(other_.chunks), sizeof(chunks));
        return *this;
    }

    BitArray& operator&=(const BitArray& other_)
    {
        for (sizet index = 0; index < kNumChunks; ++index)
        {
            chunks[index] &= other_.chunks[index];
        }
        return *this;
    }

    BitArray& operator|=(const BitArray& other_)
    {
        for (sizet index = 0; index < kNumChunks; ++index)
        {
            chunks[index] |= other_.chunks[index];
        }
        return *this;
    }

    BitArray& operator^=(const BitArray& other_)
    {
        for (sizet index = 0; index < kNumChunks; ++index)
        {
            chunks[index] ^= other_.chunks[index];
        }
        return *this;
    }

    BitArray operator~() const
    {
        BitArray copy(*this);
        copy.Flip();
        return copy();
    }

    std::strong_ordering operator<=>(const BitArray& other_) const
    {
        for (sizet index = 0; index < kNumChunks; ++index)
        {
            if (chunks[index] < other_.chunks[index])
                return std::strong_ordering::less;

            if (chunks[index] > other_.chunks[index])
                return std::strong_ordering::greater;
        }

        return std::strong_ordering::equal;
    }

    bool operator==(const BitArray& other_) const
    {
        return *this <=> other_ == std::strong_ordering::equal;
    }

    bool operator!=(const BitArray& other_) const
    {
        return *this <=> other_ != std::strong_ordering::equal;
    }

    bool operator[](sizet index_) const
    {
        return Test(index_);
    }

    bool Test(sizet index_) const
    {
        checkMsg(index_ < N, "Out of bounds access");

        sizet chunkIndex = 0;
        sizet bitIndex   = 0;

        GetIndices(index_, chunkIndex, bitIndex);

        return chunks[chunkIndex] & (1ull << bitIndex);
    }

    bool All() const
    {
        uint64 mask = u64_max;

        if constexpr (N < kNumBits)
        {
            mask = ~(1ull << (N + 1));
        }

        for (int32 index = kNumChunks - 1; index >= 0; --index)
        {
            if (chunks[index] != mask)
            {
                return false;
            }
        }

        return true;
    }

    bool Any() const
    {
        for (int32 index = kNumChunks - 1; index >= 0; --index)
        {
            if (chunks[index])
                return true;
        }

        return false;
    }

    bool None() const
    {
        return !Any();
    }

    void Set(sizet index_, bool value_)
    {
        checkMsg(index_ < N, "Out of bounds access");

        sizet chunkIndex = 0;
        sizet bitIndex   = 0;

        GetIndices(index_, chunkIndex, bitIndex);

        uint64 bit = 1ull << bitIndex;

        chunks[chunkIndex] &= ~bit;

        if (value_)
        {
            chunks[chunkIndex] |= bit;
        }
    }

    void Flip(sizet index_)
    {
        checkMsg(index_ < N, "Out of bounds access");
        sizet chunkIndex = 0;
        sizet bitIndex   = 0;

        GetIndices(index_, chunkIndex, bitIndex);

        uint64 bit = 1ull << bitIndex;

        const bool value = (chunks[chunkIndex] & bit) > 0;

        if (value)
        {
            chunks[chunkIndex] &= ~bit;
        }
        else
        {
            chunks[chunkIndex] |= bit;
        }
    }

    void Flip()
    {
        for (sizet index = 0; index < kNumChunks; ++index)
        {
            chunks[index] = ~chunks[index];
        }

        if constexpr (N < kNumBits)
        {
            const uint64 mask = ~(1ull << (N + 1));
            chunks[0] &= mask;
        }
    }

    void Reset()
    {
        Memory::Memzero(chunks);
    }

    // Returns the number of bits set to 1.
    sizet Count() const
    {
        sizet outCount = 0;
        for (int32 index = kNumChunks - 1; index >= 0; --index)
        {
            outCount += std::popcount(chunks[index]);
        }

        return std::min(outCount, N);
    }

    sizet Size() const
    {
        return N;
    }

    sizet CountLeadingZero() const
    {
        sizet outCount = 0;
        for (sizet index = 0; index < kNumChunks; ++index)
        {
            sizet leadingZeros = std::countl_zero(chunks[index]);
            if constexpr (N < kNumBits)
            {
                leadingZeros -= kNumBits - N;
            }
            outCount += leadingZeros;

            if (leadingZeros < kNumBits)
            {
                break;
            }
        }

        return std::min(outCount, N);
    }

    sizet CountLeadingOne() const
    {
        sizet outCount = 0;
        for (sizet index = 0; index < kNumChunks; ++index)
        {
            sizet leadingOnes = std::countl_one(chunks[index]);
            outCount += leadingOnes;

            if (leadingOnes < kNumBits)
            {
                break;
            }
        }

        return std::min(outCount, N);
    }

    sizet CountReverseZero() const
    {
        sizet outCount = 0;
        for (int32 index = kNumChunks - 1; index >= 0; --index)
        {
            sizet reverseZeros = std::countr_zero(chunks[index]);
            outCount += reverseZeros;

            if (reverseZeros < kNumBits)
            {
                break;
            }
        }

        return std::min(outCount, N);
    }

    sizet CountReverseOne() const
    {
        sizet outCount = 0;
        for (int32 index = kNumChunks - 1; index >= 0; --index)
        {
            sizet reverseOnes = std::countr_one(chunks[index]);
            outCount += reverseOnes;

            if (reverseOnes < kNumBits)
            {
                break;
            }
        }

        return std::min(outCount, N);
    }

  private:
    template<sizet K>
    friend class BitArray;

    friend BitArray operator&(const BitArray& lhs_, const BitArray& rhs_)
    {
        BitArray bitArray(lhs_);
        bitArray &= rhs_;
        return bitArray;
    }

    friend BitArray operator|(const BitArray& lhs_, const BitArray& rhs_)
    {
        BitArray bitArray(lhs_);
        bitArray |= rhs_;
        return bitArray;
    }

    void GetIndices(sizet index_, sizet& outChunkIndex_, sizet& outBitIndex_) const
    {
        outChunkIndex_ = kNumChunks - (index_ / kNumBits) - 1;
        outBitIndex_   = (index_ & (kNumBits - 1));
    }

    uint64 chunks[kNumChunks];
};
} // namespace Zn
