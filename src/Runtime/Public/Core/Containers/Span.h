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

#include <Core/Core.hpp>
// #include <Core/CoreAssert.h>

namespace Zn
{
template<typename T>
class Span
{
  public:
    using pointer         = T*;
    using reference       = T&;
    using const_reference = const reference;
    using size_type       = sizet;

    constexpr Span() noexcept
        : data(nullptr)
        , size(0)
    {
    }

    template<sizet N>
    constexpr Span(std::type_identity_t<T> (&arr_)[N])
        : data(DataPtr(arr_))
        , size(N)
    {
    }

    template<typename It>
    constexpr Span(It first_, sizet size_)
        : data(std::to_address(first_))
        , size(size_)
    {
    }

    template<typename Begin, typename End>
    constexpr Span(Begin begin_, End end_)
        : data(std::to_address(begin_))
        , size(end_ - begin_)
    {
    }

    constexpr Span(const Span& other_)
        : data(other_.data)
        , size(other_.size)
    {
    }

    constexpr Span(Span&& other_)
        : data(std::move(other_.data))
        , size(std::move(other_.size))
    {
    }

    constexpr Span(std::initializer_list<T>&& initList_)
        : data(std::data(initList_))
        , size(initList_.size())
    {
    }

    template<typename U, sizet N>
    constexpr Span(const std::array<U, N>& arr_)
        : data(arr_.data())
        , size(N)
    {
    }

    template<typename U, sizet N>
    constexpr Span(std::array<U, N>& arr_)
        : data(arr_.data())
        , size(N)
    {
    }

    constexpr Span& operator=(const Span& other_) = default;
    constexpr Span& operator=(Span&& other_)      = default;

    constexpr bool operator==(const Span& other_) const
    {
        return data == other_.data && size == other_.size;
    }

    constexpr bool operator!=(const Span& other_) const
    {
        return !(this->operator==(other_)());
    }

    constexpr reference operator[](sizet index_)
    {
        // checkMsg(index_ <= size, "Index out of bounds.");

        return *Memory::AddOffset(data, sizeof(T) * index_);
    }

    constexpr reference operator[](sizet index_) const
    {
        return const_cast<Span*>(this)->operator[](index_);
    }

    constexpr pointer Data() const
    {
        return data;
    }

    constexpr size_type Size() const
    {
        return size;
    }

    constexpr size_type SizeBytes() const
    {
        return size * sizeof(T);
    }

    constexpr bool IsEmpty() const
    {
        return size == 0;
    }

    constexpr Span Subspan(sizet begin_, sizet end_)
    {
        // check(begin_ < end_ && end_ - begin_ <= size, "Range out of bounds.");

        return Span(Memory::AddOffset(data, sizeof(T) * begin_), end_ - begin_);
    }

    template<typename U>
    class Iterator
    {
        U*    base;
        sizet index;
        sizet size;

      public:
        Iterator(U* base_, sizet index_, sizet size_)
            : base(base_)
            , index(index_)
            , size(size_)
        {
        }

        Iterator& operator++()
        {
            if (index < size)
            {
                ++index;
            }

            return *this;
        }

        Iterator& operator--()
        {
            if (index > 0)
            {
                --index;
            }

            return *this;
        }

        sizet Index() const
        {
            return index;
        }

        auto& operator*()
        {
            return *Memory::AddOffset(base, index * sizeof(T));
        }

        bool operator==(const Iterator& other_) const
        {
            return base == other_.base && index == other_.index && size == other_.size;
        }

        bool operator!=(const Iterator& other_) const
        {
            return !this->operator==(other_);
        }
    };

    using iterator       = Iterator<T>;
    using const_iterator = Iterator<const T>;

    iterator begin()
    {
        return iterator(data, 0, size);
    }

    iterator end()
    {
        return iterator(data, size, size);
    }

    const_iterator begin() const
    {
        return const_iterator(data, 0, size);
    }

    const_iterator end() const
    {
        return const_iterator(data, size, size);
    }

  private:
    pointer data;
    sizet   size;
};

template<typename It, typename EndOrSize>
Span(It, EndOrSize) -> Span<std::remove_reference_t<std::iter_reference_t<It>>>;

template<typename T>
Span(std::initializer_list<T>) -> Span<const T>;

template<typename T, size_t N>
Span(T (&)[N]) -> Span<T>;

template<typename T, size_t N>
Span(const T (&)[N]) -> Span<const T>;

template<typename T, size_t N>
Span(std::array<T, N>&) -> Span<T>;

template<typename T, size_t N>
Span(const std::array<T, N>&) -> Span<const T>;
} // namespace Zn
